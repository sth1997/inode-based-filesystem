#!/bin/bash
workdir=`pwd`
mountdir=${workdir}"/test_mount"
set -x
mkdir test_mount
./build/bin/inode_fuse test_mount
cd test_mount
mkdir dir1
cd dir1
echo "f1" > f1.txt
stat f1.txt
link f1.txt f2.txt
stat f1.txt
stat f2.txt
cat f2.txt
echo "f2" > f2.txt
cat f1.txt
rm f1.txt
stat f2.txt
cd ..
ln -s ${mountdir}"/dir1" dir2
ls -l dir2
cd dir2
cat f2.txt
ln -s ${mountdir}"/dir2/f2.txt" f3.txt
readlink f3.txt
echo "f3" > f3.txt
cat f2.txt
rm f2.txt
cat f3.txt
cd ..
echo "f4" > f4.txt
truncate f4.txt --size 6000000
mv f4.txt f5.txt
stat f5.txt
cd ..
fusermount -u test_mount