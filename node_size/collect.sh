grep "33554432" parse2_128.txt > collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse4_64.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse8_32.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse16_16.txt >> collect.txt
echo "----------------" >> collect.txt
grep "33554432" parse_rank.txt >> collect.txt
echo "----------------" >> collect.txt

grep "33554432" energy_parse2_128.txt > energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse4_64.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse8_32.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse16_16.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep "33554432" energy_parse_rank.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
