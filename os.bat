
call examples
vbc os.b -o boot -r 0x1000 -g os.g
vbc boot.b -o boots
vbc dir.b -o dir -G os.g
vbc exit.b -o exit -G os.g
vbc del.b -o del -G os.g
vbc copy.b -o copy -G os.g
vbc mkdir.b -o mkdir -G os.g
vbc cd.b -o cd -G os.g
vbc rmdir.b -o rmdir -G os.g
diskgen os.dsk boots boot dir exit del hello beer fizz copy mkdir cd rmdir

