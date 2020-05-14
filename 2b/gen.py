# for i in (10, 20, 40, 80):
#     for j in (1,4,8,12,16):
#         print("./lab2_list --yield=id --sync=m --lists=4 --iterations=" + str(i) + " --thread=" + str(j))
#         print("./lab2_list --yield=id --sync=s --lists=4 --iterations=" + str(i) + " --thread=" + str(j))

for i in (1,4,8,16):
    for j in (1,2,4,8,12):
        print("./lab2_list --sync=m --lists=" + str(i) + " --iterations=" + str(1000) + " --thread=" + str(j))
        print("./lab2_list --sync=s --lists=4 --iterations=" + str(1000) + " --thread=" + str(j))
