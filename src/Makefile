 # author: František Horázný
 # year: 2020
 # contact: fhorazny@gmail.com

CC=gcc
CFLAGS=-std=gnu99 -Wall -W -Wextra -pedantic -g -lm -lfftw3
RM=rm -f
NAME=voice_tracker
TEST1_DIR=../Recorder/test_1_1/
TEST2_DIR=../Recorder/test_2_2/
TEST3_DIR=../Recorder/test_3_0/
TEST4_DIR=../Recorder/test_3_1/
TEST5_DIR=../Recorder/test_3_3/
TEST6_DIR=../Recorder/test_4_6/
TEST7_DIR=../Recorder/test_30_32/
TESTCH_DIR=../Recorder/test_chuze/
TEST10_DIR=../Recorder/test0_0_1/
TEST11_DIR=../Recorder/test0_0_2/
TEST12_DIR=../Recorder/test0_0_3/
TEST13_DIR=../Recorder/test1_0_3/
TEST14_DIR=../Recorder/test1_0_3_hudba/
TEST15_DIR=../Recorder/test1_0_3_hudba_001_hlas/
TEST16_DIR=../Recorder/test1_0_4/
TEST17_DIR=../Recorder/test101_to_-101/
TEST18_DIR=../Recorder/test101hudba_001/
TEST20_DIR=../Recorder/test002_last/
TEST21_DIR=../Recorder/test003_last/
TEST22_DIR=../Recorder/test104_last/
TEST23_DIR=../Recorder/test103_speech/




all: $(NAME)

$(NAME): $(NAME).c
	$(CC) -o $(NAME) $(NAME).c $(CFLAGS)

# .PHONY: clean, run, test, test1, test2, test3, test4, test5, test6, test7, zvukovka
clean:
	$(RM) *.o $(NAME)
	
run: test1
#./$(NAME) $(TEST1_DIR)audio05.raw $(TEST1_DIR)audio01.raw $(TEST1_DIR)audio02.raw $(TEST1_DIR)audio03.raw $(TEST1_DIR)audio04.raw

help:
	@echo "make test1 - [1,1], x 58 x 58 || (0.4) test na souřadnici 0,0,1 v nezatlumené místnosti"
	@echo "make test2 - [2,2], x, 83, x, -33  ||  0.21, 0.59, 0.21, 0.23 test na souřadnici 1,0,2  (nezatlumená místnost)"
	@echo "make test3 - [3,0], x, 140, x, -140,  || x, 1, x, 1 test na souřadnici 3,0,0  (nezatlumená místnost)"
	@echo "make test4 -   x, 129, x, -115   || [3,1] test na souřadnici 2,0,1 v nezatlumené místnosti"
	@echo "make test5 -   x, 89, x, -62   || [3,3] test na souřadnici 2,0,3  (nezatlumená místnost)"
	@echo "make test6 -   x, 70, x, -53   || [4,6] test na souřadnici 3,0,6  (nezatlumená místnost)"
	@echo "make test7 -   x, -22, x, 62 -> x, 62, x, -22   || [-1,3] -> [1,3] test na souřadnici -1,0,3 postupný pohyb na 1,0,3 (nezatlumená místnost)"
	@echo "make testch - chůze směrem ke kříži u souřadnice x=0 a směrem od u souřadnice x=2"
	@echo "make test10 - v zatlumené místnosti, na souřadnicích 0,0,1 "
	@echo "make test11 - v zatlumené místnosti, na souřadnicích 0,0,2 "
	@echo "make test12 - v zatlumené místnosti, na souřadnicích 0,0,3 "
	@echo "make test13 - v zatlumené místnosti, na souřadnicích 1,0,3 "
	@echo "make test14 - v zatlumené místnosti, na souřadnicích 1,0,3 hudba "
	@echo "make test15 - v zatlumené místnosti, na souřadnicích 1_0_3 hudba a 0,0,1 hlas "
	@echo "make test16 - v zatlumené místnosti, na souřadnicích 1,0,4 "
	@echo "make test17 - v zatlumené místnosti, na souřadnicích 1,0,1 následně hned -1,0,1 "
	@echo "make test17 - v zatlumené místnosti, na souřadnicích 1,0,1 následně hned -1,0,1 "
	@echo "make test20 - v zatlumené místnosti, na souřadnicích 0,0,2 (mikrofony na zdi) "
	@echo "make test21 - v zatlumené místnosti, na souřadnicích 0,0,3 (mikrofony na zdi) "
	@echo "make test22 - v zatlumené místnosti, na souřadnicích 1,0,4 (mikrofony na zdi) "
	@echo "make test23 - v zatlumené místnosti, na souřadnicích 1,0,3 reálné mluvení (mikrofony na zdi) a druhá polovina mluvení na -1,0,2"
	@echo "make zvukovka - přeloží kód pro běh na zvukovce, heslo: adsp"
	@echo "make upload - přeloží a nahraje kód na zvukovku, heslo: adsp"
	@echo "make connect - přeloží a nahraje kód na zvukovku ... připojí na zvukovku - nutné heslo: adsp"
	@echo "make nice -n -20 ./capture_i - příkaz kterým spouštím program na zvukovce - není nutné"
	@echo "make download - stáhne textový soubor se zapsanými souřadnicemi"
	@echo "make coor - přeloží coor.c"
	
test: all
	./$(NAME)
	
zvukovka:
	/home/audified/build/fit-br2/host/usr/bin/arm-linux-gcc capture_i.c --sysroot=/home/audified/build/fit-br2/host/usr/arm-buildroot-linux-uclibcgnueabihf/sysroot -o capture_i -std=gnu99 -lm -lasound

upload:
	/home/audified/build/fit-br2/host/usr/bin/arm-linux-gcc capture_i.c --sysroot=/home/audified/build/fit-br2/host/usr/arm-buildroot-linux-uclibcgnueabihf/sysroot -o capture_i -std=gnu99 -lm -lasound
	scp capture_i adsp@147.229.14.27:/home/adsp/capture_i


connect: upload
	ssh adsp@147.229.14.27

download:
	scp adsp@147.229.14.27:/home/adsp/out_test.txt out_test.txt

coor:
	$(CC) -o coor coor.c $(CFLAGS)

test1: all
	./$(NAME) $(TEST1_DIR)audio05.raw $(TEST1_DIR)audio01.raw $(TEST1_DIR)audio02.raw $(TEST1_DIR)audio03.raw $(TEST1_DIR)audio04.raw
	@echo "test 1 [1,1], x 58 x 58 || (0.4)"

test2: all
	./$(NAME) $(TEST2_DIR)audio05.raw $(TEST2_DIR)audio01.raw $(TEST2_DIR)audio02.raw $(TEST2_DIR)audio03.raw $(TEST2_DIR)audio04.raw
	@echo "test 2 [2,2], x, 83, x, -33  ||  0.21, 0.59, 0.21, 0.23"

test3: all
	./$(NAME) $(TEST3_DIR)audio05.raw $(TEST3_DIR)audio01.raw $(TEST3_DIR)audio02.raw $(TEST3_DIR)audio03.raw $(TEST3_DIR)audio04.raw
	@echo "test 3 [3,0], x, 140, x, -140,  || x, 1, x, 1"

test4: all
	./$(NAME) $(TEST4_DIR)audio05.raw $(TEST4_DIR)audio01.raw $(TEST4_DIR)audio02.raw $(TEST4_DIR)audio03.raw $(TEST4_DIR)audio04.raw
	@echo "test 4   x, 129, x, -115   || [3,1]"

test5: all
	./$(NAME) $(TEST5_DIR)audio05.raw $(TEST5_DIR)audio01.raw $(TEST5_DIR)audio02.raw $(TEST5_DIR)audio03.raw $(TEST5_DIR)audio04.raw
	@echo "test 5   x, 89, x, -62   || [3,3]"

test6: all
	./$(NAME) $(TEST6_DIR)audio05.raw $(TEST6_DIR)audio01.raw $(TEST6_DIR)audio02.raw $(TEST6_DIR)audio03.raw $(TEST6_DIR)audio04.raw
	@echo "test 6   x, 70, x, -53   || [4,6]"

test7: all
	./$(NAME) $(TEST7_DIR)audio05.raw $(TEST7_DIR)audio01.raw $(TEST7_DIR)audio02.raw $(TEST7_DIR)audio03.raw $(TEST7_DIR)audio04.raw
	@echo "test 7   x, -22, x, 62 -> x, 62, x, -22   || [-1,3] -> [1,3]"

testch: all
	./$(NAME) $(TESTCH_DIR)audio05.raw $(TESTCH_DIR)audio01.raw $(TESTCH_DIR)audio02.raw $(TESTCH_DIR)audio03.raw $(TESTCH_DIR)audio04.raw
	@echo "test ch chůze směrem ke kříži u souřadnice x=0 a směrem od u souřadnice x=2"

test10: all
	./$(NAME) $(TEST10_DIR)audio05.raw $(TEST10_DIR)audio01.raw $(TEST10_DIR)audio02.raw $(TEST10_DIR)audio03.raw $(TEST10_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 0,0,1 "
	
test11: all
	./$(NAME) $(TEST11_DIR)audio05.raw $(TEST11_DIR)audio01.raw $(TEST11_DIR)audio02.raw $(TEST11_DIR)audio03.raw $(TEST11_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 0,0,2 "

test12: all
	./$(NAME) $(TEST12_DIR)audio05.raw $(TEST12_DIR)audio01.raw $(TEST12_DIR)audio02.raw $(TEST12_DIR)audio03.raw $(TEST12_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 0,0,3 "

test13: all
	./$(NAME) $(TEST13_DIR)audio05.raw $(TEST13_DIR)audio01.raw $(TEST13_DIR)audio02.raw $(TEST13_DIR)audio03.raw $(TEST13_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 1,0,3 "

test14: all
	./$(NAME) $(TEST14_DIR)audio05.raw $(TEST14_DIR)audio01.raw $(TEST14_DIR)audio02.raw $(TEST14_DIR)audio03.raw $(TEST14_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 1,0,3 hudba "

test15: all
	./$(NAME) $(TEST15_DIR)audio05.raw $(TEST15_DIR)audio01.raw $(TEST15_DIR)audio02.raw $(TEST15_DIR)audio03.raw $(TEST15_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 1_0_3_hudba_001_hlas "

test16: all
	./$(NAME) $(TEST16_DIR)audio05.raw $(TEST16_DIR)audio01.raw $(TEST16_DIR)audio02.raw $(TEST16_DIR)audio03.raw $(TEST16_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 1,0,4 "

test17: all
	./$(NAME) $(TEST17_DIR)audio05.raw $(TEST17_DIR)audio01.raw $(TEST17_DIR)audio02.raw $(TEST17_DIR)audio03.raw $(TEST17_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 1,0,1 to -1,0,1 "

test20: all
	./$(NAME) $(TEST20_DIR)audio05.raw $(TEST20_DIR)audio01.raw $(TEST20_DIR)audio02.raw $(TEST20_DIR)audio03.raw $(TEST20_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích 0,0,2"

test21: all
	./$(NAME) $(TEST21_DIR)audio05.raw $(TEST21_DIR)audio01.raw $(TEST21_DIR)audio02.raw $(TEST21_DIR)audio03.raw $(TEST21_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích "

test22: all
	./$(NAME) $(TEST22_DIR)audio05.raw $(TEST22_DIR)audio01.raw $(TEST22_DIR)audio02.raw $(TEST22_DIR)audio03.raw $(TEST22_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích "

test23: all
	./$(NAME) $(TEST23_DIR)audio05.raw $(TEST23_DIR)audio01.raw $(TEST23_DIR)audio02.raw $(TEST23_DIR)audio03.raw $(TEST23_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích "

test24: all
	./$(NAME) $(TEST24_DIR)audio05.raw $(TEST24_DIR)audio01.raw $(TEST24_DIR)audio02.raw $(TEST24_DIR)audio03.raw $(TEST24_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích "

test25: all
	./$(NAME) $(TEST25_DIR)audio05.raw $(TEST25_DIR)audio01.raw $(TEST25_DIR)audio02.raw $(TEST25_DIR)audio03.raw $(TEST25_DIR)audio04.raw
	@echo "test v zatlumené místnosti, na souřadnicích "

test26: all
	./$(NAME) $(TEST26_DIR)audio05.raw $(TEST26_DIR)audio01.raw $(TEST26_DIR)audio02.raw $(TEST26_DIR)audio03.raw $(TEST26_DIR)audio04.raw
	@echo "test"
	

