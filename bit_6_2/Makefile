game:game.o Main.o PracBar.o
	gcc $^ -o $@
%.o:%.c
	gcc $< -c
.PHONY:clean
clean:
	rm -f *.o game
