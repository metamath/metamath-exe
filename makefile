objects = metamath.o mmcmdl.o mmcmds.o mmdata.o mmhlpa.o mmhlpb.o mminou.o mmmaci.o mmpars.o mmpfas.o mmunif.o mmutil.o mmveri.o mmvstr.o \
	mmword.o mmwtex.o mmhtbl.o mmwsts.o


metamath: $(objects)
	cc -o metamath $(CFLAGS) $(objects)

%.o: %.c
	cc -c $*.c -std=gnu99 $(CFLAGS)


