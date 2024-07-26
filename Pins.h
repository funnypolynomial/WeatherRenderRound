#pragma once

// Schematic
// 
//        +-----------------+
//  [SDA]-+(SDA)  <1>  (SCL)+-[SCL]             +--------+            
//        |                 |                   | BME280 |               SET <4>   
//   [CS]-+A0    LEO     ~D9+-[BLK]             |        |               ---       
//        |      TINY       |            [5VDC]-+VIN     |        [GND]--   --[BTN]
//  [LDR]-+A1           ~D10+-[DC]              |        |
//        |                 |             [GND]-+GND     |
//  [BTN]-+A2           ~D11+-[RES]             |        |               +---------+       +------+
//        |                 |             [SCL]-+SCL     |      [5VDC]---+ LDR <5> +---+---| 22kR |---[GND]
// [5VDC]-+"+"           "0"+-[GND]             |        |               +---------+   |   +------+
//        |                 |             [SDA]-+SDA     |                             |
// [MOSI]-+(MOSI) <2>   (SCK)+-[SCK]             +--------+                             +---[LDR]
//        +------|USB|------+
//
//
//            ***********
//          *             *
//        *                 *
//       *                   *---------+
//      *                     *     GND+-[GND]
//      *                     *     VCC+-[5VDC]
//      *                     * <3> SCL+-[SCK]
//      *        LCD          * <3> SDA+-[MOSI]
//      *      240x240        *     RES+-[RES]
//      *   (GC9a01 1.28in)   *      DC+-[DC]
//      *                     *      CS+-[CS]
//      *                     *     BLK+-
//       *                   *---------+
//        *                 *
//          *             *
//            ***********
//
// Notes:
// Leonardo Tiny, https://www.jaycar.co.nz/leonardo-tiny-atmega32u4-main-board/p/XC4431
// Matching [LABELS] are connected.
//  <1>: SDA & SCL are pads on the underside of the Leo
//  <2>: MOSI & SCK are ICSP pads on the underside of the Leo
//  <3>: LCD's SDA/SCL labels are misleading -- it's SPI not I2C
//  <4>: Push-button, momentary closed
//  <5>: LDR is ~220R bright, ~20MR dark 


#define PIN_BLK 9
#define PIN_DC  10
#define PIN_RES 11
#define PIN_CS  A0
#define PIN_LDR A1
#define PIN_BTN A2
