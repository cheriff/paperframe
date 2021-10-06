import datetime
import pickle
import math
import os
from io import BytesIO
from typing import List, Tuple

import pytz
import cairo
import rsvg

from weather_fetch import WeatherClient
from holidays import holidays

from widget import WidgetBase
from widget import BLACK, WHITE, GREEN, BLUE, RED, YELLOW, ORANGE

PICKLE=os.getenv( "PICKLE" )

class Weather( WidgetBase ):
    def __init__(self, api_key, lat, long, timezone):
        self.api_key = api_key
        self.lat = lat
        self.long = long
        self.timezone = pytz.timezone(timezone)
        WidgetBase.__init__( self )

    def fetch( self ):

        # Fetch weather

        self.weather = None
        if PICKLE:
            try:
                self.weather = pickle.load( open( PICKLE, "rb" ) )
                print( "dehydrated: ", PICKLE)
            except FileNotFoundError:
                pass

        if not self.weather:
            print("Fetching..")
            self.weather = WeatherClient(self.lat, self.long, self.timezone)
            self.weather.load(self.api_key)
            print("fetched")
            if PICKLE:
                pickle.dump( self.weather, open( PICKLE, "wb" ) )
                print( "preserved: ", PICKLE)

    def render(self):
        self.fetch()

        #with cairo.ImageSurface(cairo.FORMAT_ARGB32, 600, 448) as surface:
        with self.render_begin as context:
            print ("I have context: ", context )
            # Draw features
            self.draw_date(context)
            self.draw_temps(context)
            self.draw_column(context, self.weather.hourly_summary(0), 120, 30)
            self.draw_column(context, self.weather.hourly_summary(2 * 3600), 120, 155)
            self.draw_column(context, self.weather.hourly_summary(5 * 3600), 120, 280)
            self.draw_column(context, self.weather.daily_summary(1), 120, 440)
            self.draw_meteogram(context)
            self.draw_alerts(context)
            self.draw_stats(context)

        return self.output

    def draw_temps(self, context: cairo.Context):
        # Draw on temperature ranges
        temp_min, temp_max = self.weather.temp_range_24hr()
        # Draw background rects
        self.draw_roundrect(context, 360, 5, 75, 65, 5)
        context.set_source_rgb(*BLUE)
        context.fill()
        self.draw_roundrect(context, 520, 5, 75, 65, 5)
        context.set_source_rgb(*RED)
        context.fill()
        self.draw_text(
            context,
            position=(395, 55),
            text=round(temp_min),
            color=WHITE,
            weight="bold",
            size=50,
            align="center",
        )
        self.draw_text(
            context,
            position=(475, 55),
            text=round(self.weather.temp_current()),
            color=BLACK,
            weight="bold",
            size=50,
            align="center",
        )
        self.draw_text(
            context,
            position=(558, 55),
            text=round(temp_max),
            color=WHITE,
            weight="bold",
            size=50,
            align="center",
        )

    def draw_meteogram(self, context: cairo.Context):
        top = 310
        left = 10
        width = 425
        height = 85
        left_axis = 18
        hours = 24
        y_interval = 10
        graph_width = width - left_axis
        # Establish function that converts hour offset into X
        hour_to_x = lambda hour: left + left_axis + (hour * (graph_width / hours))
        # Draw day boundary lines
        today = self.weather.hourly_summary(0)["day"]
        for hour in range(hours):
            day = self.weather.hourly_summary(hour * 3600)["day"]
            if day != today:
                context.save()
                context.move_to(hour_to_x(hour) - 0.5, top)
                context.line_to(hour_to_x(hour) - 0.5, top + height)
                context.set_line_width(1)
                context.set_source_rgb(*BLACK)
                context.set_dash([1, 1])
                context.stroke()
                context.restore()
                today = day
        # Establish temperature-to-y function
        temps = [
            self.weather.hourly_summary(hour * 3600)["temperature"]
            for hour in range(hours + 1)
        ]
        temp_min = min(temps)
        temp_max = max(temps)
        scale_min = math.floor(temp_min / y_interval) * y_interval
        scale_max = math.ceil(temp_max / y_interval) * y_interval
        temp_to_y = lambda temp: top + (scale_max - temp) * (
            height / (scale_max - scale_min)
        )
        # Draw rain/snow curves
        precip_to_y = lambda rain: top + 1 + (max(4 - rain, 0) * (height / 4))
        rain_points = []
        snow_points = []
        for hour in range(hours + 1):
            conditions = self.weather.hourly_summary(hour * 3600)
            rain_points.append((hour_to_x(hour), precip_to_y(conditions["rain"])))
            snow_points.append((hour_to_x(hour), precip_to_y(conditions["snow"])))
        self.draw_precip_curve(
            context, points=rain_points, bottom=precip_to_y(0), color=BLUE
        )

        # Draw value lines
        for t in range(scale_min, scale_max + 1, y_interval):
            y = temp_to_y(t)
            context.move_to(left + left_axis, y + 0.5)
            context.line_to(left + left_axis + graph_width, y + 0.5)
            context.set_line_width(1)
            context.set_source_rgb(*BLACK)
            context.save()
            context.set_dash([1, 1])
            context.stroke()
            context.restore()
            self.draw_text(
                context,
                text=t,
                position=(left + left_axis - 6, y),
                size=14,
                color=BLACK,
                align="right",
                valign="middle",
            )
        # Draw temperature curve
        for hour in range(hours + 1):
            conditions = self.weather.hourly_summary(hour * 3600)
            if hour == 0:
                context.move_to(hour_to_x(hour), temp_to_y(conditions["temperature"]))
            else:
                context.line_to(hour_to_x(hour), temp_to_y(conditions["temperature"]))
        context.set_source_rgb(*WHITE)
        context.set_line_width(6)
        context.stroke_preserve()
        context.set_source_rgb(*RED)
        context.set_line_width(3)
        context.stroke()
        # Draw hours and daylight/UV bar
        bar_top = top + height + 13
        for hour in range(hours + 1):
            conditions = self.weather.hourly_summary(hour * 3600)
            x = hour_to_x(hour)
            # Hour label
            if hour % 3 == 0 and hour < hours:
                self.draw_text(
                    context,
                    text=conditions["hour"],
                    position=(x, bar_top + 19),
                    size=15,
                    align="center",
                    valign="bottom",
                )
            # Conditions bar
            if hour < hours:
                color = BLACK
                if conditions["uv"]:
                    color = ORANGE
                if conditions["uv"] > 7:
                    color = RED
                context.rectangle(x, bar_top, (graph_width / hours) + 1, 8)
                context.set_source_rgb(*color)
                context.fill()

    def draw_column(self, context: cairo.Context, conditions, top, left):
        # Heading
        if "date" in conditions:
            time_text = (
                conditions["date"].astimezone(self.timezone).strftime("%A").title()
            )
        else:
            time_text = (
                conditions["time"].astimezone(self.timezone).strftime("%-I%p").lower()
            )
        self.draw_text(
            context,
            text=time_text,
            position=(left + 50, top + 25),
            color=BLACK,
            size=28,
            align="center",
        )
        self.draw_icon(context, conditions["icon"], (left, top + 33))

    def draw_alerts(self, context: cairo.Context):
        # Load weather alerts
        alerts = self.weather.active_alerts()
        for alert in alerts:
            alert["color"] = RED

        # Add holidays
        for holiday_date, holiday_name in holidays.items():
            days_until = (holiday_date - datetime.date.today()).days
            if 0 < days_until <= 30:
                alerts.append(
                    {
                        "text": holiday_name,
                        "subtext": (
                            "in %i days" % days_until if days_until != 1 else "tomorrow"
                        ),
                        "color": BLUE,
                    }
                )
        top = 265
        left = 5
        for alert in alerts:
            text = alert["text"]
            text_width = self.draw_text(
                context,
                text,
                position=(0, 0),
                size=20,
                weight="bold",
                noop=True,
            )
            self.draw_roundrect(context, left, top, text_width + 15, 30, 4)
            context.set_source_rgb(*alert["color"])
            context.fill()
            left += self.draw_text(
                context,
                text,
                position=(left + 8, top + 23),
                size=20,
                color=WHITE,
                weight="bold",
            )
            if alert.get("subtext"):
                subtext_width = self.draw_text(
                    context,
                    alert["subtext"],
                    position=(left + 20, top + 26),
                    size=15,
                    color=BLACK,
                )
            else:
                subtext_width = 0
            left += 30 + subtext_width

    def draw_stats(self, context: cairo.Context):
        # Draw sunrise, sunset icon and values
        self.draw_icon(context, "rise-set-aqi", (450, 337))
        self.draw_text(
            context,
            position=(505, 372),
            text=self.weather.sunrise().astimezone(self.timezone).strftime("%H:%M"),
            color=BLACK,
            size=32,
        )
        self.draw_text(
            context,
            position=(505, 422),
            text=self.weather.sunset().astimezone(self.timezone).strftime("%H:%M"),
            color=BLACK,
            size=32,
        )

    def draw_precip_curve(
        self,
        context: cairo.Context,
        points: List[Tuple[int, int]],
        bottom: int,
        color,
        curviness: float = 7,
    ):
        # Draw the top curves
        for i, point in enumerate(points):
            if i == 0:
                context.move_to(*point)
            else:
                last_point = points[i - 1]
                context.curve_to(
                    last_point[0] + curviness,
                    last_point[1],
                    point[0] - curviness,
                    point[1],
                    point[0],
                    point[1],
                )
        # Draw the rest and fill
        context.line_to(points[-1][0], bottom)
        context.line_to(points[0][0], bottom)
        context.close_path()
        context.set_source_rgb(*color)
        context.fill()

if __name__ == "__main__":
    composer = Weather(
        api_key=os.getenv('APIKEY'),
        lat=-33.889648, long=151.176422,
        timezone="Australia/Sydney"
     )
    img = composer.render()
    with open( "out.png", "wb") as f:
        f.write( img.getbuffer())
    print("Wrote: out.png")

