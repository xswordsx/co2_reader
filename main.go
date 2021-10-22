// Binary co2_reader_go reads data off of a CCS811 Gas sensor
// and exposes the data as a Prometheus-formatted HTTP endpoint.
package main

import (
	"fmt"
	"log"
	"net/http"
	"os"
	"sync"
	"time"

	"periph.io/x/conn/v3/i2c/i2creg"
	"periph.io/x/devices/v3/ccs811"
	"periph.io/x/host/v3"
)

// Using
const promDesc = `
# HELP sensor_co2_ppm Shows how many parts per million of the ambient atmosphere are CO2
# TYPE sensor_co2_ppm gauge
sensor_co2_ppm %d

# HELP sensor_voc_ppm Shows how many parts per million of the ambient atmosphere are VOC
# TYPE sensor_voc_ppm gauge
sensor_voc_ppm %d
`

var (
	// mux guards the sensor data.
	mux sync.RWMutex = sync.RWMutex{}

	// sensorData are sequential reads of the CCS811 sensor.
	sensorData *ccs811.SensorValues = &ccs811.SensorValues{}

	// l is the default application logger.
	l *log.Logger = log.New(os.Stdout, "   ", log.Ltime|log.Ldate|log.Lmsgprefix)
)

func main() {
	wg := sync.WaitGroup{}
	wg.Add(1)

	l.Println("Starting CO2 sensor reader")
	go func() {
		if err := run(&wg); err != nil {
			l.Fatal(err)
		}
	}()

	wg.Wait()

	l.Println("Hooking up Prometheus metrics endpoint")
	http.HandleFunc("/metrics", func(rw http.ResponseWriter, r *http.Request) {
		mux.RLock()
		d := *sensorData
		mux.RUnlock()
		fmt.Fprintf(rw, promDesc, d.ECO2, d.VOC)
	})

	l.Println("Serving metrics on http://localhost:2112/metrics")
	if err := http.ListenAndServe("0.0.0.0:2112", nil); err != nil {
		l.Printf("HTTP Server error: %v", err)
	}
}

func run(wg *sync.WaitGroup) error {
	// Load all the drivers:
	if _, err := host.Init(); err != nil {
		log.Fatal(err)
	}
	// Open a handle to the first available I²C bus:
	bus, err := i2creg.Open("")
	if err != nil {
		return err
	}
	defer bus.Close()

	sensorSettings := &ccs811.Opts{
		Addr:            0x5A,
		MeasurementMode: ccs811.MeasurementModeConstant1000,
	}

	l.Printf("Sensor will be run with settings: %+v", *sensorSettings)
	dev, err := ccs811.New(bus, sensorSettings)
	if err != nil {
		return err
	}
	fwData, err := dev.GetFirmwareData()
	if err != nil {
		return err
	}

	l.Println("=== Hardware information ===")
	l.Printf("Hardware ID:         0x%x", fwData.HWIdentifier)
	l.Printf("Hardware version:    0x%x", fwData.HWVersion)
	l.Printf("Boot version:        %s", fwData.BootVersion)
	l.Printf("Application version: %s", fwData.ApplicationVersion)

	if fwData.HWIdentifier != 0x81 {
		return fmt.Errorf("unknown hardware ID: 0x%x", fwData.HWIdentifier)
	}

	if err := dev.StartSensorApp(); err != nil {
		return err
	}
	time.Sleep(50 * time.Microsecond)

	wg.Done()

	for {
		// Wait for data to be available
		for (sensorData.Status & 0b1001000) == 0 {
			b, err := dev.ReadStatus()
			if err != nil {
				return err
			}
			sensorData.Status = b
			time.Sleep(time.Second)
		}

		mux.Lock()
		err := dev.SensePartial(ccs811.ReadCO2VOC, sensorData)
		mux.Unlock()

		if err != nil {
			return err
		}

		time.Sleep(time.Second)
	}
}
