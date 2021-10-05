#png2png

A conversion utility for Paperframe.

###Usage:
```
$ ./png2png IN_FILE OUT_FILE
```

Load an input image, and write an equivalent output in a format suitable for paperframe. I believe the generated files to be compliant (`pngcheck` seems to agree) and these are the only files regularly tested against with Paperframe.

# Discussion

The emitted file is of the same dimensions as the `IN_FILE`, although for use with Paperframe this should be restricted to 600x448.

The input file can have any pixel format, and the output is always written in 4 bits per pixel colourmap mode, with a pallete which roughly matches how the physical e-paper display looks to my eyes.

Currently no attempt at quantization or dithering is made. Instead each pixel is compared to a list of colour constants in order to determine which pallete index to use. A concession is made for an existing image generation tool which has slightly different ideas about what RED, BLUE, etc mean as compared to the pallete I use. Due to this each pixel is compared to two possible vairants.

Despite the name, `IN_FILE` may be any image format (supported by [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)) and not only .png. In practice, lossy file formats like .jpg may result in colours which are not exactly on the allowed pallete and this could cause problems, due to lack of smarter quantization.

Every single pixel *must* be one of the following colours, otherwise the tool will fail. It is currently up to you to preprocess your images such that this is the case.

For some convinience, each colour has two suitable representations in the input image: Literal, which more easily matches ideal colours, and Realistic, which is a bit closer to the display physically appears so is a good choice for dithering algorithms.

| Colour | Index | Literal | Realistic |
| ------ | ----- | ------- | --------- |
| BLACK  | 0     |`#000000`| `#0c0c0e` |
| WHITE  | 1     |`#ffffff`| `#d2d2d0` |
| GREEN  | 2     |`#00ff00`| `#1e601f` |
| BLUE   | 3     |`#0000ff`| `#1d1e54` |
| RED    | 4     |`#ff0000`| `#8c1b1d` |
| YELLOW | 5     |`#ffff00`| `#d3c93d` |
| ORANGE | 6     |`#ff9100`| `#c1712a` |