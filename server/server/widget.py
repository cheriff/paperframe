import datetime
import pickle
import math
import os
from io import BytesIO
from typing import List, Tuple

import pytz
import cairo
import rsvg

BLACK = (0, 0, 0)
WHITE = (1, 1, 1)
GREEN = (0, 1, 0)
BLUE = (0, 0, 1)
RED = (1, 0, 0)
YELLOW = (1, 1, 0)
ORANGE = (1, 0.57, 0)

from contextlib import contextmanager

@contextmanager
def SurfaceManager( widget ):
    with cairo.ImageSurface(cairo.FORMAT_ARGB32, 600, 448) as surface:
        widget.surface = surface
        context = cairo.Context(surface)
        context.set_antialias( cairo.Antialias.NONE )

        fo = context.get_font_options()
        fo.set_antialias( cairo.Antialias.NONE )
        context.set_font_options( fo )

        context.rectangle(0, 0, 600, 448)
        context.set_source_rgb(1, 1, 1)
        context.fill()

        yield context

        widget.output = BytesIO()
        surface.write_to_png(widget.output)

class WidgetBase( object ):
    def __init__( self ):
        pass

    @property
    def render_begin( self ):
        return SurfaceManager( self )

    def draw_date(self, context: cairo.Context, now = None):
        now = now or datetime.datetime.now(self.timezone)
        # Day name
        left = 5
        self.draw_text(
            context,
            text=now.strftime("%A"),
            position=(left, 55),
            size=60,
            weight="light",
        )
        # Day number
        left += self.draw_text(
            context,
            text=now.strftime("%d").lstrip("0"),
            position=(left, 90),
            size=30,
            color=BLACK,
            weight="bold",
        )
        th = {
            "01": "st",
            "02": "nd",
            "03": "rd",
            "21": "st",
            "22": "nd",
            "23": "rd",
            "31": "st",
        }.get(now.strftime("%d"), "th")
        left += 1
        left += self.draw_text(
            context,
            text=th,
            position=(left, 75),
            size=15,
            color=BLACK,
            weight="bold",
        )
        # Month name (short)
        left += 5
        left += self.draw_text(
            context,
            text=now.strftime("%B"),
            position=(left, 90),
            size=30,
            color=BLACK,
            weight="bold",
        )

    def draw_roundrect(self, context, x, y, width, height, r):
        context.move_to(x, y + r)
        context.arc(x + r, y + r, r, math.pi, 3 * math.pi / 2)
        context.arc(x + width - r, y + r, r, 3 * math.pi / 2, 0)
        context.arc(x + width - r, y + height - r, r, 0, math.pi / 2)
        context.arc(x + r, y + height - r, r, math.pi / 2, math.pi)
        context.close_path()

    def draw_text(
        self,
        context: cairo.Context,
        text: str,
        size: int,
        position: Tuple[int, int] = (0, 0),
        color=BLACK,
        weight="regular",
        align="left",
        valign="top",
        noop=False,
    ):
        text = str(text)
        if weight == "light":
            context.select_font_face("Roboto Light")
        elif weight == "bold":
            context.select_font_face(
                "Roboto", cairo.FontSlant.NORMAL, cairo.FontWeight.BOLD
            )
        else:
            context.select_font_face("Roboto")
        context.set_source_rgb(*color)
        context.set_font_size(size)
        xbear, ybear, width, height = context.text_extents(text)[:4]
        if align == "right":
            x = position[0] - width - xbear
        elif align == "center":
            x = position[0] - (width / 2) - xbear
        else:
            x = position[0]
        if valign == "middle":
            y = position[1] + (height / 2)
        elif valign == "bottom":
            y = position[1] + height
        else:
            y = position[1]
        if not noop:
            context.move_to(x, y)
            context.show_text(text)
        return width

    def draw_icon(self, context: cairo.Context, icon: str, position: Tuple[int, int]):
        print ( "LOADING ICON", icon )
        path = os.path.join(os.path.dirname(__file__), "icons-7", f"{icon}.svg")
        h = rsvg.Handle( path )

        image = cairo.ImageSurface(
            cairo.FORMAT_ARGB32,
            h.props.width,
            h.props.height)
        c = cairo.Context(image)
        c.set_antialias( cairo.Antialias.NONE )
        context.set_antialias( cairo.Antialias.NONE )
        h.render_cairo(c)

        context.save()
        context.translate(*position)
        context.set_source_surface(image)
        context.paint()
        context.restore()
