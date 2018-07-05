grep -P "\t1000\t"  parse4M.txt > collect.txt
echo "----------------" >> collect.txt
grep -P "\t1000\t"  parse8M.txt >> collect.txt
echo "----------------" >> collect.txt
grep -P "\t1000\t"  parse16M.txt >> collect.txt
echo "----------------" >> collect.txt
grep -P "\t1000\t"  parse32M.txt >> collect.txt
echo "----------------" >> collect.txt
grep -P "\t1000\t"  parse64M.txt >> collect.txt
echo "----------------" >> collect.txt
grep -P "\t1000\t"  parse128M.txt >> collect.txt
echo "----------------" >> collect.txt
grep -P "\t1000\t"  parse256M.txt >> collect.txt
echo "----------------" >> collect.txt

grep -P "\t1000\t"  energy_parse4M.txt > energy_collect.txt
echo "----------------" >> energy_collect.txt
grep -P "\t1000\t"  energy_parse8M.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep -P "\t1000\t"  energy_parse16M.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep -P "\t1000\t"  energy_parse32M.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep -P "\t1000\t"  energy_parse64M.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep -P "\t1000\t"  energy_parse128M.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt
grep -P "\t1000\t"  energy_parse256M.txt >> energy_collect.txt
echo "----------------" >> energy_collect.txt


