# CO2 sensor reader

This little applicaiton reads data off of a [CCS811 Gas sensor](https://www.sciosense.com/products/environmental-sensors/ccs811-gas-sensor-solution/) and exposes the data to a Prometheus endpoint.

## Building

    $ CGO_ENABLED=0 go build .

## Running

    $ ./co2_reader_go

## License

This project is licensed under the [MIT license](https://mit-license.org/)

## Issues

Use the repository [issues board](https://github.com/xswordsx/co2_reader_go/issues)
