./vbc os.b -o boot -r 0x1000 -g os.g
./vbc boot.b -o boot.out
./vbc dir.b -o dir -G os.g
rm os.g
./diskgen os.dsk boot.out boot dir hello.out beer.out fizz.out
