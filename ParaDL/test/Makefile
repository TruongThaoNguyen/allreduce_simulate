vgg: vgg16_o

vgg16_para: VGG16.para.c
	/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpicc VGG16.para.c -o VGG16_para.run

alexnet_para: ALEXNET.para.c
	/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpicc ALEXNET.para.c -o alexnet_para.run	
	
resnet: resnet50_o resnet50_para

allreduce:
	/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpicc allreduce.c -o allreduce.run

resnet50_para: RESNET50.para.c
	/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpicc RESNET50.para.c -o RESNET50_para.run
	
resnet50_o: RESNET50.c
	/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpicc RESNET50.c -o RESNET50.run
	
resnet50_overlap: RESNET50_overlap.c
	/home/nguyen_truong/allreduce_simulate/SimGrid-3.19/build/bin/smpicc RESNET50_overlap.c -o RESNET50_overlap.run	
	
clean:
	/bin/rm -f *.run
	/bin/rm -f *.log*
	/bin/rm -f smpitmp*

