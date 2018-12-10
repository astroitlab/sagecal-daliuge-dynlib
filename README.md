SAGECAL on Daliuge with DynlibAppDrop
=====================================
#### Prerequsites::
    DALiuGe
    SAGECAL

#### Compile
    1, create a soft link "lib" to the directory ligsagecal.a existed(ln -s).
    2, update Makefile if dependent libs were installed in different directories (like casacore, openBLAS).
    3, make all

#### Test
    The json file(dist_sagecal_dynlib.json) is the logical graph and includes self-customized paths(like DynlibDrop's libpath) which you need to modify according to your path.
    Copy the dist_sagecal_dynlib.json to LogicGraph's directory. Deploy following DALiuGe's instructions.
