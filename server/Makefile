
include private.mk

venv/pyvenv.cfg:
	virtualenv -p python3 venv
	. ./venv/bin/activate && pip install pytz pycairo tzdata

.PHONY: run
run: | venv/pyvenv.cfg
	. ./venv/bin/activate && PICKLE=weather.pickle python server/weather.py

.PHONY: serve
serve: | venv/pyvenv.cfg
	@echo KEY IS $(APIKEY)
	#. ./venv/bin/activate && PICKLE=weather.pickle python server/serve.py
	. ./venv/bin/activate && python server/serve.py


