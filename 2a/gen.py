for j in (1, 2, 4, 8, 12, 16, 24):
    print("./lab2_list --sync=m --iterations=" + str(1000) + " --thread=" + str(j))
    print("./lab2_list --sync=s --iterations=" + str(1000) + " --thread=" + str(j))
