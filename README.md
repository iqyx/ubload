uBLoad (uMeshFw bootloader)
===================================

What it is and what it isn't
--------------------------------

uBLoad's main three goals are:

  * **to provide a safe way of upgrading your firmware remotely**. This includes checking the
    firmware integrity, storing multiple firmware versions in an external SPI flash memory
    and running a failover mechanism if upgrading or bootloading procedure fails
    (eg. when a wrong firmware image has been flashed or if corruption occured)

  * **secure you against possible remote firmware modification** in case your remote firmware
    upload/upgrade service fails and allows an attacker to bypass your authentication and
    upload his modified firmware image. Every firmware image is authenticated before booting
    using asymmetric signatures with write-only public keys stored in the bootloader.
    Such firmware will simply fail to boot and the failover mechanism will be triggered.

  * **secure you against local firmware modification/upgrade done by a low skilled attacker**
    (common users count) when physical access to your device is gained.

uBLoad **won't** help you when:

  * your application is so vulnerable that it allows an attacker to do remote code execution
    and/or overwrite parts of itself or the bootloader with modified code and run it
    directly.

  * your goal is to protect your intellectual property. uBLoad was not designed to do it
    and this goal cannot be safely done. Firmware image is not encrypted and even if it was,
    there would be no possibility of decrypting it safely (without using shared keys or
    generating per-device asymmetrically encrypted images), which threatens security
    (symmetric keys can be extracted) or usability.



Main features:
---------------------------

  * designed for STM32 Cortex-M3/M4 microcontrollers
  * nice command line interface over the USART peripheral (serial console)
  * multiple firmware images can be stored in a SPI NOR flash
  * firmware integrity checking on boot (SHA512 hash)
  * asymmetric (Ed25519) firmware authentication on boot, keys are stored write-only
    directly in the bootloader and cannot be reasonably modified without erasing the
    bootloader itself.
  * failover action can be executed if any of the check fails (eg. loading the last
    known working firmware version)
  * XMODEM protocol support for downloading new firmware images over the serial console
  * LED diagnostic of the bootloading process

Planned features:

  * DFU support for firmware uploading
  * virtual serial console over USB


Documentation
--------------------

This text can be found on the [project page](http://qyx.krtko.org/projects/ubload)
(warning, recursion!). Further information about the bootloader can be found on the
[bootloader internals page](internals.md). Firmware usage and examples are documented
in the [user manual](user.md).


Licensing
--------------------

uBLoad is licensed under the GPLv3 license with some parts licensed under the simplified
(2 clause) BSD. User application code is not considered derived work even if you are
shipping both uBLoad and the user application in a single binary file. Therefore, your
code doesn't need to be open sourced as is normally the case with the GPL and static linking.
However, changes in the uBLoad itself have to be disclosed if you are shipping modified
uBLoad binaries with your product.

Reasoning behing this is - if you need to hide something which was meant to be secure,
you are doing it wrong. Use your own bootloader in this case.




Source code and contributing
--------------------------------

You can grab the sources on the [GitHub repository](https://github.com/iqyx/ubload).
If you have a modification, feature or a fix you would like to share, feel free to open
new issue or send a pull request.


