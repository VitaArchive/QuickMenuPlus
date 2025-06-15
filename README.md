# Quick Menu Plus

Quick Menu Plus adds the following features to the Quick Menu on the PlayStation Vita and PlayStation TV.

- Power off, restart, and standby buttons
- Configurable power buttons
- Volume slidebar
- Faster Quick Menu open time
- Custom background styles

[![quickmenuplus-preview.png](https://git.shotatoshounenwachigau.moe/vita/quickmenuplus/plain/quickmenuplus-preview-small.png?h=assets)](https://git.shotatoshounenwachigau.moe/vita/quickmenuplus/plain/quickmenuplus-preview.png?h=assets)

## Installation

Supported firmware versions:

- Retail 3.60-3.73
- Testkit 3.60, 3.65
- Devkit 3.60

Write the file path in the `*main` section of your taiHEN config.

```
*main
ur0:/tai/quickmenuplus.suprx
```

## Configuration

Put files in the directory `ur0:/data/quickmenuplus` to configure Quick Menu Plus.

- `pushtime.txt` sets the Quick Menu PS button push time. Put in the file the desired push time in microseconds. For example, put `250000` for 250 milliseconds. If the file does not exist, the default time is 500 milliseconds.

- `standbyisrestart.txt` sets the standby button to function as a restart button. Put in the file `1` to enable, or `0` to disable. If the file does not exist, the default is enabled on the Vita, and disabled on the PSTV. When this setting is disabled, hold the "Power Off・Restart" button to restart, and press to power off.

    [![standbyisrestart-preview.png](https://git.shotatoshounenwachigau.moe/vita/quickmenuplus/plain/standbyisrestart-preview-small.png?h=assets)](https://git.shotatoshounenwachigau.moe/vita/quickmenuplus/plain/standbyisrestart-preview.png?h=assets)

- `bgstyle.txt` sets the background style. Put in the file `0` for original, `1` for translucent, and `2` for black. If the file does not exist, the default is translucent. Black can reduce power consumption for OLED screens. The gradient effect is removed in all styles.

    [![bgstyle-preview.png](https://git.shotatoshounenwachigau.moe/vita/quickmenuplus/plain/bgstyle-preview-small.png?h=assets)](https://git.shotatoshounenwachigau.moe/vita/quickmenuplus/plain/bgstyle-preview.png?h=assets)

## Building

Dependencies:

- [DolceSDK](https://forum.devchroma.nl/index.php/topic,129.0.html)
- [psp2dbg](https://git.shotatoshounenwachigau.moe/vita/psp2dbg)
- [taiHEN](https://git.shotatoshounenwachigau.moe/vita/taihen)

Logging can be configured with CMake variables.

To build dependencies and module:

```sh
cmake .
make dep-all
make
```

## Contributing

Use [git-format-patch](https://www.git-scm.com/docs/git-format-patch) or [git-request-pull](https://www.git-scm.com/docs/git-request-pull) and email me at <asakurareiko@protonmail.ch>.

## Credits

- Princess-of-Sleeping: Tip for SceShellUtil, ScePaf

## See also

- [Discussion](https://forum.devchroma.nl/index.php/topic,78.0.html)
- [Source repository](https://git.shotatoshounenwachigau.moe/vita/quickmenuplus)
