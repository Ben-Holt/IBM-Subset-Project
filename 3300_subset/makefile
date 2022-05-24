make:
	gcc -Wall sim.c -o subset

run:
	reset && ./subset -v < simple_add.in

test:
	reset
	gcc -Wall sim.c -o subset
	./subset -v < regular_loop.in > regularout
	./subset -v < simple_loop.in > simpleout
	./subset -v < compare.in > compareout
	diff regularout regular_loop.verbose
	diff simpleout simple_loop.verbose
	diff compareout compare.verbose
	./subset < simple_loop.in > simpleout
	diff simpleout simple_loop.out
	./subset < bad_bct.in > badbctout
	diff badbctout bad_bct.out
	./subset < bad_load.in > badloadout
	diff badloadout bad_load.out
	./subset -v < oor_pc.in > oorpcout
	diff oorpcout oor_pc.verbose
	./subset -v < simple_add.in > simpleaddout
	diff simpleaddout simple_add.verbose

clean:
	rm subset
	rm compareout
	rm regularout
	rm simpleout
	rm badbctout
	rm badloadout
	rm oorpcout
	rm simpleaddout
	

	