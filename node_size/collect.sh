grep "33554432" parse2.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse4.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse8.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse16.txt >> collect.txt
echo "----------------" >> collect.txt

grep "33554432" energy_parse2.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse4.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse8.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse16.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt

