# Hardware

## Parts per light

| Part | DigiKey | Notes |
|---|---|---|
| Seeed XIAO ESP32-C3 | 1597-113991054-ND | ~$5. Wi-Fi, USB-C, 21 x 17.8 mm. Comes with a flex antenna on a U.FL lead; no headers. |
| 12-LED NeoPixel ring, **RGB** | 1528-1102-ND (Adafruit 1643) | Saturated colors. Firmware: `xiao_rgb_wifi` or `xiao_rgb_usb`. |
| 12-LED NeoPixel ring, **RGBW** | 1528-1604-ND natural ~4500K (2852), 1528-1605-ND warm ~3000K (2851), 1528-1603-ND cool ~6000K (2853) | Adds a real white LED: clean whites and pastels. Firmware: `xiao_rgbw_wifi` or `xiao_rgbw_usb`. |
| 470 ohm resistor | makerspace | On the data wire, close to the ring. |
| 220-470 uF capacitor, 6.3 V+ | makerspace | Across ring power, optional but recommended. |
| USB-C **data** cable | any | Powers the light; also the USB fallback and for flashing. |
| 3M Bumpon SJ5302 | 3M156269-ND | Feet. |

All 12-LED rings are 36.8 mm across, 23.3 mm inside, 3.6 mm thick.

## Wiring

```
XIAO 5V  ──────────────────────────── ring PWR (5V)
XIAO GND ──────────────────────────── ring GND
XIAO D2  ── 470 ohm (near the ring) ── ring IN      (never OUT)
            capacitor across ring PWR/GND (stripe = GND)
```

- D2 is GPIO4. Avoid D0, D8, D9 (start-up pins) and D6/D7 (serial).
- The ring runs at 5 V from USB; the XIAO sends 3.3 V data. With short wires NeoPixels
  almost always accept that. If colors flicker or the first LED misbehaves, add a level
  shifter (74AHCT125) or move the ring's power to 3V3 (dimmer but in spec).
- Stick the flex antenna to an inside wall, flat, away from the ring's power wires and
  any metal. Plug its lead into the XIAO's U.FL socket before closing anything up.
- Brightness is capped at 60/255 in firmware, well inside what USB can supply.

## Choosing RGB or RGBW

RGB is fine for bold blue/amber/green. RGBW also does soft pastels, warm glows and real
white, which is what the mushroom light uses. The firmware supports both; flash the matching
image. Colors are defined once in `firmware/src/looks.cpp` as {r, g, b, w} and look similar
on both (on RGB the white part is mixed from r, g and b).
