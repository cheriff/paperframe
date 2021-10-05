# Image Server

This example server displays the weather.

I previously had some janky framework for writing text and boxes and lines, but frankly it was terrible. Nor do I have much in the way of artistic design skills so early results were not very encouraging.

Fortunately I came across a similar project which had already done this hard work: weatherboard

* [Twitter: @andrewgodwin](https://twitter.com/andrewgodwin/status/1423431373033857025)
* [Github: andrewgodwin/weatherboard](https://github.com/andrewgodwin/weatherboard)
* [Author's blog](https://aeracode.org/2020/06/22/e-paper-weather-display/)

I didnt need the Linux/Raspberry Pi code for displaying the image, but weatherboard contains a nice class which uses python Cairo bindings to render a nice display.

Similarly wrapped behind a web server I only had a few minor changes to make:

* Use `rsvg` to obtain a Cairo surface directly from the original icon .svg files. This avoids the need to raster them out to png through something like inkscape.
* Disable antialiasing for the font rendering
* Disable antialiasing for general cairo drawing
* Hand-edit some of the .svg icons, to play nice with rsvg.
  * be (more?) explicit width and height
  * `shape-rendering="optimizeSpeed"` which is apparently rsvg-speak for disable anti-aliasing on the new surface.
  * For some reason I use a different shade of orange
* Slight layout tweaks to personal preference
   * Only want/need temperature C	
   * AQI data doesnt seem available outside US

The end result is that generated images consist only of 7 target colours for the display. This avoids the server returning a nicely antialiased image which looks good on a monitor, but then having the shades thrown away when reducing down to the 7-colour pallete. I'm not sure if this is equivalent (or better, or worse) to a good dither on the original image, but it avoids the need to do so.

For a rough idea of what's needed at runtime, take a look at the Dockerfile or the Makefile, both of which seem to work for me.