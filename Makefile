
.PHONY : usage

usage :
	@echo "SELECT THE BUILD TYPE"


# BUILD TYPES
unix_sdl2:
	@make -f unix_sdl2.mak

unix_sdl:
	@make -f unix_sdl.mak

win_sdl2:
	@make -f win_sdl2.mak

ps2_sdl:
	@make -f ps2_sdl.mak

gc_devkitppc:
	@make -f gc_devkitppc.mak


clean:
	rm -rf ./objs



