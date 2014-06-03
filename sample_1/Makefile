EXEC    = sppCtrl
FILES        = sppCtrl.c status.c sample.c interface.c shutils.c utils.c

CFLAGS += -I./include

all: 
	$(CC) $(FILES) -o $(EXEC) $(CFLAGS) $(LDFLAGS)
#	$(CC) $(FILES) -o $(EXEC) -I./include -DX86_TEST

clean:
	      rm $(EXEC)
