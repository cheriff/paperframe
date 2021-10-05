FROM python:3.9-alpine

RUN apk add build-base cairo-dev cairo cairo-tools jpeg-dev zlib-dev freetype-dev lcms2-dev openjpeg-dev tiff-dev tk-dev tcl-dev librsvg-dev
RUN pip install requests pytz pycairo tzdata
ADD server /server/
EXPOSE 8003

CMD ["python3", "/server/serve.py"]
