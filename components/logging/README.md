
This is a simple logger, based on [https://github.com/rxi/log.c]()

Some changes have been made to be a little more suitable for the embedded environment.

Things like:

* Use rpi-pico's `board_ms()` instead of a full date/time format string
* Collapse into single header-only file