NCAM=`grep 81e6 /proc/pci | wc -l`
NDRIVER=`grep pvpci /lib/modules/misc | wc -l`




echo Pvcam installation: $NCAM Roper Scientific Photometrics type cameras found.


x=0
while [ $x -lt $NCAM ]
do
# make a node for each PCI card installed as pvcam0 c(char) major=61 minor=0, pvcam1....
  mknod /dev/pvcam$x c 61 $x
  x=`expr $x + 1`
done

if [ $NCAM ]
then
# now copy and install the new module
  mkdir /lib/modules/misc
  cp ./pvpci.ko /lib/modules/misc
  insmod -f pvpci.ko
  cp ./libpvcam.so.2.7.0.0 /usr/lib/
  ln -s /usr/lib/libpvcam.so.2.7.0.0 /usr/lib/libpvcam.so
  ldconfig /usr/lib
fi

echo Photometrics Device Driver and PVCAM for Linux installation complete.
echo Please Reboot, before running the examples.






