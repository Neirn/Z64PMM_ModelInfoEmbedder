# Z64PMM_ModelInfoEmbedder
A simple command line utility for embedding metadata into a zobj for Z64Recomp_PlayerModelManager.

# How to Use
The program accepts two arguments, a path to a zobj and path to a json. Simply pass those two arguments in any order.

The zobj is expected to be a zobj that ran through zzplayas/z64playas with one of ModLoader64's manifests.

To see an example json, see the example.json in the repo.

If the json is valid and the zobj is valid, then the metadata is directly written into some unused space the zobj.

Example run:
```
./modelinfoembedder_windows.exe mycoolmodel.zobj mycoolmodelinformation.json
```
