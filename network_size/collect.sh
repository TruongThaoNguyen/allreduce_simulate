grep "33554432" parse32.txt > collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse64.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse128.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse256.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse512.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse1024.txt >> collect.txt
echo "----------------" >> collect.txt

grep "33554432" energy_parse32.txt > energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse64.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse128.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse256.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse512.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse1024.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
