# CO2 sensor reader

This little applicaiton reads data off of a [CCS811 Gas sensor](https://www.sciosense.com/products/environmental-sensors/ccs811-gas-sensor-solution/) and exposes the data to a Prometheus endpoint.

## Installing

```
go get github.com/xswordsx/2co_reader/go
```

## Running

Current project presumptions:

* The sensor runs in its default mode (address `0x5A` for the registry).
* The sensor does _NOT_ use the `nINT` signal wire.

```
$ ./co2_reader_go
```

## Building locally

```
$ go get ./... && go build .
```

## License

This project is licensed under the [MIT license](https://mit-license.org/)

## Issues

Use the repository [issues board](https://github.com/xswordsx/co2_reader/issues)
