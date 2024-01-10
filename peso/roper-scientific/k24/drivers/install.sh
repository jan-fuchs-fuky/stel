NCAMPI=`grep 801d /proc/pci | wc -l` 
NDRIVERPI=`grep pipci /proc/modules | wc -l`
NALIASESPI=`grep pipci /etc/modules.conf | wc -l`

echo "--------------------------"
echo " Instalace modulù pipci.o "
echo "--------------------------"

if [ $NDRIVERPI -gt 0 ]
then
  rmmod pipci
fi

cp -v alias177 /etc

if [ $NALIASESPI -eq 0 ]
then
  cat alias177 >> /etc/modules.conf
fi   

if [ $NCAMPI ]
then
  mknod /dev/rspipci0 c 177 0
  mkdir -v /lib/modules/misc
  cp -v pipci.o /lib/modules/misc
fi
