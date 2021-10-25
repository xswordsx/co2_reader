# CO2 sensor reader

This little applicaiton reads data off of a [CCS811 Gas sensor](https://www.sciosense.com/products/environmental-sensors/ccs811-gas-sensor-solution/)
and prints it to stdout in a CSV format.

## Installing

1. Make sure you have `dev-i2c` installed
1. Download the source
1. Run `./make.sh`
   - For debug builds (more logging) you could run `./make.sh -D_DEBUG_`

## Running

Current project presumptions:

* The sensor runs in its default mode (address `0x5A` for the registry).
* The sensor does _NOT_ use the `nINT` signal wire.

```
$ ./build/co2_reader
```

## License

This project is licensed under the [MIT license](https://mit-license.org/)

## Issues

Use the repository issues board.
