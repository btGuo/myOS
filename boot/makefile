OBJS = ../build/boot.o
AS = as
ASFLAGS = --32 

all:$(OBJS)

$(OBJS):boot.S
	$(AS) $(ASFLAGS) -o $(OBJS) boot.S	
