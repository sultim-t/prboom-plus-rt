OBJS = i_main.o i_net.o i_sound.o i_system.o i_video.o mmus2mid.o ser_main.o

include ../rules.make

libplatform.o: $(OBJS)
	ld -i $(OBJS) -o libplatform.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

