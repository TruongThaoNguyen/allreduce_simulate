# overall result
grep "finished" ./logs/* >> parse.log
#Replace \./.*:Dense blocking finished \[0\] with \n$0
#Replace Ring_Ring --> Ring Ring
#Replace Allreduce --> 
#Replace blocking --> 
#Replace  finished \[[0-9]\]: --> \t
#Replace ./logs/ --> ./logs/\t
#Replace .log: --> \t
#Replace \t\t --> \t

#detail
grep "Sparse Ring Ring" ./logs/* >> detail.log
#Replace Dense: False with $0\n
