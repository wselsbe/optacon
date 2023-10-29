
# Optacon

This project aims to recreate the [Optacon](https://en.wikipedia.org/wiki/Optacon) (OPtical to TActile CONverter), a reading aid for the blind. The user moves a small camera module over a document to scan one letter at a time. The main module has a "tactile display": a 6x24 array of vibrating pins, so the user can feel the shape of the letter being scanned with their fingertip.

The vibrating pins are actuated by piezoelectric bimorphs, similar to those used in refreshable braille displays. The Optacon uses vibrating pins instead of static, and achieves a higher pin density: 1mm x 2mm versus 2.5mm x 2.5mm.
The following videos show the display in action. The piezo bimorphs actuate at around 250Hz, the motion appears slowed down by a stroboscopic led.

[![optacon pins](http://img.youtube.com/vi/8xHrjtj89DE/0.jpg)](http://www.youtube.com/watch?v=8xHrjtj89DE)
[![optacon pins](http://img.youtube.com/vi/HoEaBOcu1ug/0.jpg)](http://www.youtube.com/watch?v=HoEaBOcu1ug)

The high density tactile display remains state of the art. Even now, 50 years later, there is no technology available that achieves the same high density while still remaining portable and battery powered.

# Recreating the tactile display
The smallest commercially available piezoelectric actuators are those meant for braille cells, and are 2.5mm wide. No piezo manufacturer wanted to try to create narrower actuators.

My solution is to start with an off the shelf 40mm x 20mm x 0.9mm actuator, and dice it into 20 individual parallel actuators with a [wafer dicing saw](https://www.eevblog.com/forum/reviews/micro-automation-m1006-dicing-saw). This uses a very precise air bearing spindle with a very thin (55 micron) diamond blade to cut through the piezoceramic. Because the blade is water cooled, the piezoceramic actuator remains below the curie temperature (about 150°C) and does not lose its polarisation.

## Holding and mounting the piezoceramic actuator for dicing
Holding the actuators during and after dicing proved challenging. Typically silicon wafers are mounted with [UV dicing tape](https://www.adwill-global.com/en/tape/dicing-d.html), but this tape is expensive.
Another potential solution is [heat release tape](https://www.nitto.com/eu/en/products/e_parts/electronic001/), where the adhesive softens at higher temperature (but still below the piezoceramic curie temperature), but this is similarly expensive.

I got some success by using [Crystalbond 509](https://www.aremco.com/mounting-adhesives/): by heating it to 74°C on a hotplate I could apply the adhesive to a glass disc, add the piezoceramic, and letting the adhesive cool down to harden. The disc is then held in place the the dicing saw's vacuum chuck. But transferring the diced actuators to a pcb, while still remaining parallel after dicing, proved to be too difficult. The Crystalbond 509 has to be released by using heat and a solvent (acetone), without affecting the second adhesive used to attach the piezoceramic reeds to the pcb.

The key proved to be in reversing the steps.

1. First populate the pcb and reflow solder as usual
2. Apply the conductive adhesive on the pcb actuator pads with a stencil. I'm using MG Chemicals [8331S](https://mgchemicals.com/products/adhesives/electrically-conductive-adhesives/electrically-conductive-paste/) 2-part or [9410](https://mgchemicals.com/products/adhesives/electrically-conductive-adhesives/conductive-adhesive/) 1-part silver epoxy.
3. Attach the piezoceramic and heat-cure the epoxy on a hot plate or in a reflow (toaster) oven. The above epoxies can cure below the curie temperature.
4. Mount the pcb to the glass disc with [Crystalbond 555-HMP](https://www.aremco.com/mounting-adhesives/). Although 555 has much lower tensile strength compared to 509, the larger surface area of the pcb is enough to keep it in place.
   The silver epoxy provides enough shear strengths to hold the individual actuators on one end. By applying the Crystalbond 555 on the underside of the piezoceramic through the slots in the pcb, the free ends of the diced piezoceramic reeds remain supported.
5. Dice the piezoceramic.
6. Crystalbond 555 is water soluble, any adhesive exposed to the water cooling from the dicing blade will be removed quickly while dicing. To detach the pcb from the glass disc it can be placed in a heated ultrasonic cleaner.


# Electronics

## Actuator board

The actuator board is designed around 2 components: a high voltage piezo driver, and high voltage shift registers .
All components are chosen for small size and low height, to keep the boards stackable. The tallest component is the inductor at 1.2mm. The passive components are 0603 for manual pick and place.

### Piezo Driver

The [TI DRV2665](https://www.ti.com/product/DRV2665) can output 100V with only an inductor and capacitor. It has an I2C interface and an analog input. 
Alternatives are [DRV2667](https://www.ti.com/product/DRV2667) (pin compatible) and [DRV8667](https://www.ti.com/product/DR8667) & [DRV2700](https://www.ti.com/product/DRV2700) (analog only)
The inductor [TDK VLS3012HBX-3R3M](https://product.tdk.com/en/search/inductor/inductor/smd/info?part_no=VLS3012HBX-3R3M) is chosen for low height, low resistance (150mΩ), and high current (2.37A temp rating).
The capacitor is rated for 200V.
TI has an [excel sheet](https://www.ti.com/tool/download/SLOR113) to help calculate the values for the inductor, capacitor, and feedback resistors.

To increase the efficiency the piezo driver is powered from VIN (battery or usb). The I2C lines are pulled up to VDD (3.3V), compatible down to 1.8V

The piezo driver has a differential output, which can drive piezos at 200V peak to peak. Because of the shift registers only one output (up to 100V) is used.
Without input the outputs float at half the boost voltage, ideally the external controller should make sure the input is pulled down.

The I2C and analog input pins are broken out to the FFC connector.

### Shift Registers

The [Microchip HV509](https://www.microchip.com/en-us/product/HV509) high voltage shift registers are used to address the individual piezo actuators. There are no real alternatives available in QFN or similar low height package.
The common BP pin is connected to the common bottom & top electrodes on the piezos. The individual outputs go to the center electrodes. There are 16 outputs per shift registers, 10 are used on each shift register, the other 6 are unused.
Diode D1 is needed to bias the output transistors to at least VDD before the high voltage is enabled.
The datasheet says the minimum voltage is 50V for the outputs to switch. In practice this does not look like a major issue: if the HV is a sine wave they might switch late, but will still pass through the lower voltage after switching. (Needs further testing)

Both shift registers are chained together, the SPI pins (clock, data in, data out) and Latch and Polarity are broken out to the FFC connector

## Motherboard

The motherboard is designed to drive several actuator boards (up to 6), a future revision will accept external input such as a camera to read printed letters.
It's based on an ESP32-S3-WROOM-1 module.
Currently the goal of the motherboard is to act as a controller and interface to the actuator boards. At first, the firmware should simply generate a sinewave and step through the individual actuators, as a test for the actuator boards.