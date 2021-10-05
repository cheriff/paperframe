
A Streaming PNG decoder
=

Single-pass decoder for a PNG image.

Right now is overly tuned for the current usecase - decoding images for presentation on an e-paper display, but with some refactoring could become more generic.

It's not too amazing or robust, but does the trick for me right now.

Usage
==
1. zero-initialize a `pngStream_t` object
2. make one or more calls to `process_sector`

You can call just once with a whole file already in memory, or make many individual calls as data becomes available.

Quirks[^1]
==
* Enforces e-paper limitations (resolution, colour depth, etc)
* Doesnt really report error (just assertions)
* Use of static X/Y coordinates in `flush_imgBuff` isn't idea, I suspect this is what prevents multiple images from being sent without a reboot in between. These should be elsewhere and re-zeroable.
* Directly calls out to `epd_*` routines to drive the epaper display. These should probably be callback based insuead.
* Only supports unfiltered (type 0) PNG rows.
* No indication on when processing is complete, and probably wont handle being given extra bytes that well. It is up to the caller to (try not to) under or overflow.

[^1]: aka: things I know are _bad_ but it still kind of works, so ¯\_(ツ)_/¯