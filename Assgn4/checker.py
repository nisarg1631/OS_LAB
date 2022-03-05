lines = []

with open("output.txt", "r") as f:
    lines = f.readlines()

# for line in lines:
#     print(line)

adjlist = []
color = []
parent = []
vis = []
for _ in range(1500):
    adjlist.append([])
    color.append(0)
    vis.append(0)
    parent.append(-1)

for line in lines:
    comm = line.split()
    if comm[0] == '+':
        adjlist[int(comm[2])].append(int(comm[1]))

cnt = 0

def dfs(curr_node, parent_node):
    global cnt, adjlist, color, parent, vis
    cnt += 1
    vis[curr_node] = 1
    parent[curr_node] = parent_node
    for child in adjlist[curr_node]:
        if vis[child] == 1:
            print("Incorrect")
        dfs(child, curr_node)

dfs(0,-1)

cnt1 = 0
cnt2 = 0

for line in lines:
    comm = line.split()
    cur = int(comm[1])
    par = int(comm[2])
    if comm[0] == '~':
        if color[cur] != 0:
            print("Incorrect1")
        if par != -1 and color[par] != 0:
            print("Incorrect2")
        if par != parent[cur]:
            print("Incorrect3")
        if len(adjlist[cur]) != 0:
            print("Incorrect7")
        color[cur] = 1
        cnt1 += 1
    if comm[0] == '-':
        if color[cur] != 1:
            print("Incorrect4")
        if par != -1 and color[par] != 0:
            print("Incorrect5")
        if par != parent[cur]:
            print("Incorrect6")
        adjlist[par].remove(cur)
        color[cur] = 2
        cnt2 += 1
        

print(cnt)
print(cnt1)
print(cnt2)
