autoconfig := include/config/auto.conf

deps_config := \
	Kconfig \
	modules/Kconfig \
	modules/test/Kconfig \
	modules/test2/Kconfig \

$(autoconfig): $(deps_config)
$(deps_config): ;
