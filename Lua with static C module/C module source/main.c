/* COMPILE WITH
* LIBTOOL="libtool --tag=CC --silent"
* $LIBTOOL --mode=compile cc -c module.c
* $LIBTOOL --mode=link cc -rpath /usr/local/lib/lua/5.1 -o libmodule.la module.lo
* mv .libs/libmodule.so.0.0.0 module.so

* gcc -Wall -shared -fPIC -o power.so  -I/usr/include/lua5.1 -llua5.1   hellofunc.c
*/
#define LUA_LIB
#include "/usr/include/lua5.1/lua.h"
#include "/usr/include/lua5.1/lualib.h"
#include "/usr/include/lua5.1/lauxlib.h"
#include <stdlib.h>
#include <wiringPi.h>

#define MAXLEN 256

#define INP_FILENAME "inputs.txt"
#define OUT_FILENAME "outputs.txt"
#define SIG_FILENAME "signs.txt"


typedef struct
{
 int X;
 int Y;
 int Z;
 int VAL;
} Block;

/*LUA functions*/
static int worldTick(lua_State *L);
static int l_blockOn(lua_State *L);   //Set the block at X, Y, Z in world World to On state
static int l_blockOff(lua_State *L);   //Set the block at X, Y, Z in world World to Off state
static int Quitting(lua_State *L);
static int LogIO(lua_State *L);
static int l_addInput(lua_State *L);
static int l_addOutput(lua_State *L);
static int l_addSign(lua_State *L);
static int l_remInput(lua_State *L);
static int l_remOutput(lua_State *L);
static int l_remSign(lua_State *L);


int blockOn(lua_State *L, int x, int y, int z);   //Set the block at X, Y, Z in world World to On state
int blockOff(lua_State *L, int x, int y, int z);   //Set the block at X, Y, Z in world World to Off state
int blockState(lua_State *L, int x, int y, int z);   //Return the block state
int pcall_catch(lua_State *L, const char *description, int nargs, int nrets, int err);
int saveBlockArray(Block *ar, int len, const char *filename);   //returns 0 for OK, -n for error
int loadBlockArray(Block *ar, int maxlen, const char *filename);    //returns number of blocks read or -n for error
int sortedFindBlock(Block *ar, int arlen, int x, int y, int z); //Find block index in a sorted array
int addInput(int x, int y, int z, int pin);
int addOutput(int x, int y, int z, int pin);
int addSign(int x, int y, int z, int type);
int remInput(int x, int y, int z);
int remOutput(int x, int y, int z);
int remSign(int x, int y, int z);
int BlockCompare(const void* p1, const void* p2);   //Compare two blocks by coordinates (for sorting and searching)
void rem_block(Block *ar, int index, int arlen);

Block inputs[MAXLEN];
Block outputs[MAXLEN];
Block signs[MAXLEN];
int n_inputs;
int n_outputs;
int n_signs;

int boardToWiringPi[26] = {-1, -1, 8, -1, 9, -1, 7, 15, -1, 16, 0, 1, 2, -1, 3, 4, -1, 5, 12, -1, 13, 6, 14, 10, -1, 11};

int luaopen_MCmodule(lua_State *L)
{
 lua_register(
            L,               /* Lua state variable */
			"worldTick",        /* func name as known in Lua */
			worldTick          /* func name in this file */
			);
 lua_register(L, "blockOn", l_blockOn);
 lua_register(L, "blockOff", l_blockOff);
 lua_register(L, "Quitting", Quitting);
 lua_register(L, "LogIO", LogIO);

 lua_register(L, "addInput", l_addInput);
 lua_register(L, "addOutput", l_addOutput);
 lua_register(L, "addSign", l_addSign);

 lua_register(L, "remInput", l_remInput);
 lua_register(L, "remOutput", l_remOutput);
 lua_register(L, "remSign", l_remSign);

 n_inputs = loadBlockArray(inputs, MAXLEN, INP_FILENAME);
 n_outputs = loadBlockArray(outputs, MAXLEN, OUT_FILENAME);
 n_signs = loadBlockArray(signs, MAXLEN, SIG_FILENAME);
 wiringPiSetup();

 printf("Loaded -- inputs: %d, outputs: %d, signs: %d.\n", n_inputs, n_outputs, n_signs);
 return 0;
}

static int Quitting(lua_State *L)
{
 if (saveBlockArray(inputs, MAXLEN, INP_FILENAME) != 0)
 {
  printf("MCmodule: Error saving inputs array!\n");
 }
 if (saveBlockArray(outputs, MAXLEN, OUT_FILENAME) != 0)
 {
  printf("MCmodule: Error saving outputs array!\n");
 }
 if (saveBlockArray(signs, MAXLEN, SIG_FILENAME) != 0)
 {
  printf("MCmodule: Error saving signs array!\n");
 }
 return 0;
}

static int LogIO(lua_State *L)
{
 printf("Logging IO. Inputs: %d, outputs: %d, signs: %d.\n", n_inputs, n_outputs, n_signs);
 for (int i = 0; i < n_inputs; i++)
 {
  printf("Input %d, [%d,%d,%d]: PIN%d.\n", i, inputs[i].X, inputs[i].Y, inputs[i].Z, inputs[i].VAL);
 }
 for (int i = 0; i < n_outputs; i++)
 {
  printf("Output %d, [%d,%d,%d]: PIN%d.\n", i, outputs[i].X, outputs[i].Y, outputs[i].Z, outputs[i].VAL);
 }
 for (int i = 0; i < n_signs; i++)
 {
  printf("Sign %d, [%d,%d,%d]: PIN%d.\n", i, signs[i].X, signs[i].Y, signs[i].Z, signs[i].VAL);
 }
 return 0;
}


static int worldTick(lua_State *L)
{
 for (int i = 0; i < n_inputs; i++)
 {
  pinMode(boardToWiringPi[inputs[i].VAL], INPUT);
  if (digitalRead(boardToWiringPi[inputs[i].VAL]) == 1)
  {
   blockOn(L, inputs[i].X, inputs[i].Y, inputs[i].Z);
  } else
  {
   blockOff(L, inputs[i].X, inputs[i].Y, inputs[i].Z);
  }
 }
 for (int i = 0; i < n_outputs; i++)
 {
  pinMode(boardToWiringPi[outputs[i].VAL], OUTPUT);
  digitalWrite(boardToWiringPi[outputs[i].VAL], blockState(L, outputs[i].X, outputs[i].Y, outputs[i].Z));
 }
 return 0;
}


static int l_blockOn(lua_State *L)    //Should ba called as blockOn(World,X,Y,Z)
{
 int x = lua_tointeger(L, 2);
 int y = lua_tointeger(L, 3);
 int z = lua_tointeger(L, 4);

 lua_pop(L, 3);    //remove the coords from stack

 lua_pushinteger(L, blockOn(L, x, y, z));           /* Push the return */
 return 1;                              /* One return value */
}

static int l_blockOff(lua_State *L)    //Should ba called as blockOff(World,X,Y,Z)
{
 int x = lua_tointeger(L, 2);
 int y = lua_tointeger(L, 3);
 int z = lua_tointeger(L, 4);

 lua_pop(L, 3);    //remove the coords from stack

 lua_pushinteger(L, blockOff(L, x, y, z));           /* Push the return */
 return 1;                              /* One return value */
}

int blockState(lua_State *L, int x, int y, int z)
{
 //Get the block info
 int type, meta;

 lua_getfield(L, 1, "GetBlockInfo"); /* method of World (which is stored at index 4)*/
 lua_pushvalue(L, 1);
 lua_pushinteger(L, x);                        /* 1st argument */
 lua_pushinteger(L, y);                        /* 2nd argument */
 lua_pushinteger(L, z);                        /* 3rd argument */

 pcall_catch(L, "World:SetBlock", 4, 5, 0);

 type = lua_tointeger(L, -4);
 meta = lua_tointeger(L, -3);
 //Remove the new vars from the stack
 lua_pop(L, 5);

 //printf("Block: [%d,%d,%d],\n", blockX, blockY, blockZ);
 //printf("-info: v,t,m,sl,bl: %d, %d, %d, %d, %d.\n", valid, type, meta, skylight, blocklight);

 switch (type)
 {
  case 69:  //lever
  case 77:  //stone button
  case 143: //wood button
    return meta >> 7;    //return the eight bit

  case 76:  //active redstone torch
  case 124: //active redstone lamp
    return 1;

  case 75:  //inactive redstone torch
  case 123:  //inactive redstone lamp
    return 0;
  default:
    return 0;
 }
}

int blockOn(lua_State *L, int x, int y, int z)
{
 int OK = 1;    //the return value

 //Get the block info
 int type, meta;

 lua_getfield(L, 1, "GetBlockInfo"); /* method of World (which is stored at index 4)*/
 lua_pushvalue(L, 1);
 lua_pushinteger(L, x);                        /* 1st argument */
 lua_pushinteger(L, y);                        /* 2nd argument */
 lua_pushinteger(L, z);                        /* 3rd argument */

 pcall_catch(L, "World:SetBlock", 4, 5, 0);

 type = lua_tointeger(L, -4);
 meta = lua_tointeger(L, -3);
 //Remove the new vars from the stack
 lua_pop(L, 5);

 //printf("Block: [%d,%d,%d],\n", blockX, blockY, blockZ);
 //printf("-info: v,t,m,sl,bl: %d, %d, %d, %d, %d.\n", valid, type, meta, skylight, blocklight);

 switch (type)
 {
  case 69:  //lever
  case 77:  //stone button
  case 143: //wood button
    meta |= 0x8;    //make the block be ON
    lua_getfield(L, 1, "SetBlockMeta"); /* function to be called */
    lua_pushvalue(L, 1);    //push World onto stack
    lua_pushinteger(L, x);                        /* 1st argument */
    lua_pushinteger(L, y);                        /* 2nd argument */
    lua_pushinteger(L, z);                        /* 3rd argument */
    lua_pushinteger(L, meta);                        /* 4th argument */
    pcall_catch(L, "World:SetBlockMeta", 5, 0, 0);
    break;
  case 75:  //inactive redstone torch
    lua_getfield(L, 1, "SetBlock"); /* function to be called */
    lua_pushvalue(L, 1);    //push World onto stack
    lua_pushinteger(L, x);                        /* 1st argument */
    lua_pushinteger(L, y);                        /* 2nd argument */
    lua_pushinteger(L, z);                        /* 3rd argument */
    lua_pushinteger(L, 76);                        /* 4th argument */
    lua_pushinteger(L, meta);                        /* 5th argument */
    pcall_catch(L, "World:SetBlock", 6, 0, 0);
    break;
  case 123:  //inactive redstone lamp
    lua_getfield(L, 1, "SetBlock"); /* function to be called */
    lua_pushvalue(L, 1);    //push World onto stack
    lua_pushinteger(L, x);                        /* 1st argument */
    lua_pushinteger(L, y);                        /* 2nd argument */
    lua_pushinteger(L, z);                        /* 3rd argument */
    lua_pushinteger(L, 124);                         /* 4th argument */
    lua_pushinteger(L, meta);                        /* 5th argument */
    pcall_catch(L, "World:SetBlock", 6, 0, 0);
    break;
  default: OK = 0; break;   //did not change anything, return 0
 }
 return OK;
}

int blockOff(lua_State *L, int x, int y, int z)    //Should ba called as blockOff(X,Y,Z,World)
{
 int OK = 1;    //the return value

 //Get the block info
 int type, meta;

 lua_getfield(L, 1, "GetBlockInfo"); /* method of World (which is stored at index 4)*/
 lua_pushvalue(L, 1);
 lua_pushinteger(L, x);                        /* 1st argument */
 lua_pushinteger(L, y);                        /* 2nd argument */
 lua_pushinteger(L, z);                        /* 3rd argument */

 pcall_catch(L, "World:SetBlock", 4, 5, 0);

 type = lua_tointeger(L, -4);
 meta = lua_tointeger(L, -3);
 //Remove the new vars from the stack
 lua_pop(L, 5);

 //printf("Block: [%d,%d,%d],\n", blockX, blockY, blockZ);
 //printf("-info: v,t,m,sl,bl: %d, %d, %d, %d, %d.\n", valid, type, meta, skylight, blocklight);
 switch (type)
 {
  case 69:  //lever
  case 77:  //stone button
  case 143: //wood button
    meta &= 0x7;    //make the block be ON
    lua_getfield(L, 1, "SetBlockMeta"); /* function to be called */
    lua_pushvalue(L, 1);    //push World onto stack
    lua_pushinteger(L, x);                        /* 1st argument */
    lua_pushinteger(L, y);                        /* 2nd argument */
    lua_pushinteger(L, z);                        /* 3rd argument */
    lua_pushinteger(L, meta);                        /* 4th argument */
    pcall_catch(L, "World:SetBlockMeta", 5, 0, 0);
    break;
  case 76:  //active redstone torch
    lua_getfield(L, 1, "SetBlock"); /* function to be called */
    lua_pushvalue(L, 1);    //push World onto stack
    lua_pushinteger(L, x);                        /* 1st argument */
    lua_pushinteger(L, y);                        /* 2nd argument */
    lua_pushinteger(L, z);                        /* 3rd argument */
    lua_pushinteger(L, 75);                        /* 4th argument */
    lua_pushinteger(L, meta);                        /* 5th argument */
    pcall_catch(L, "World:SetBlock", 6, 0, 0);
    break;
  case 124:  //active redstone lamp
    lua_getfield(L, 1, "SetBlock"); /* function to be called */
    lua_pushvalue(L, 1);    //push World onto stack
    lua_pushinteger(L, x);                        /* 1st argument */
    lua_pushinteger(L, y);                        /* 2nd argument */
    lua_pushinteger(L, z);                        /* 3rd argument */
    lua_pushinteger(L, 123);                         /* 4th argument */
    lua_pushinteger(L, meta);                        /* 5th argument */
    pcall_catch(L, "World:SetBlock", 6, 0, 0);
    break;
  default: OK = 0; break;   //did not change anything, return 0
 }

 return OK;
}

static int l_addInput(lua_State *L)
{
 int blockX = lua_tointeger(L, 1);
 int blockY = lua_tointeger(L, 2);
 int blockZ = lua_tointeger(L, 3);
 int pin = lua_tointeger(L, 3);

 printf("MCmodule: Input added in C.\n");
 addInput(blockX, blockY, blockZ, pin);

 return 0;
}
static int l_addOutput(lua_State *L)
{
 int blockX = lua_tointeger(L, 1);
 int blockY = lua_tointeger(L, 2);
 int blockZ = lua_tointeger(L, 3);
 int pin = lua_tointeger(L, 3);

 addOutput(blockX, blockY, blockZ, pin);

 return 0;
}
static int l_addSign(lua_State *L)
{
 int blockX = lua_tointeger(L, 1);
 int blockY = lua_tointeger(L, 2);
 int blockZ = lua_tointeger(L, 3);
 int type = lua_tointeger(L, 3);

 addSign(blockX, blockY, blockZ, type);

 return 0;
}

static int l_remInput(lua_State *L)
{
 int blockX = lua_tointeger(L, 1);
 int blockY = lua_tointeger(L, 2);
 int blockZ = lua_tointeger(L, 3);

 remInput(blockX, blockY, blockZ);

 return 0;
}
static int l_remOutput(lua_State *L)
{
 int blockX = lua_tointeger(L, 1);
 int blockY = lua_tointeger(L, 2);
 int blockZ = lua_tointeger(L, 3);

 remOutput(blockX, blockY, blockZ);

 return 0;
}
static int l_remSign(lua_State *L)
{
 int blockX = lua_tointeger(L, 1);
 int blockY = lua_tointeger(L, 2);
 int blockZ = lua_tointeger(L, 3);

 remSign(blockX, blockY, blockZ);

 return 0;
}



//The function must already be prepared with its arguments and all!!
int pcall_catch(lua_State *L, const char *description, int nargs, int nrets, int err)
{
 if (lua_pcall(L, nargs, nrets, err) != 0)  /* do the call */
 {
  printf("MCmodule: Error running function '%s': %s\n", description, lua_tostring(L, -1));
  return 0;
 } else
 {
  return 1;
 }
}

int saveBlockArray(Block *ar, int len, const char *filename)
{
 int ret = 0;
 FILE *fp;
 fp = fopen(filename, "w");
 if (!fp) return -1;
 if (fwrite(ar, sizeof(Block), len, fp) != len) //Not all bytes written
 {
  ret = -2;
 }
 if (fclose(fp) != 0)   //File not closed succesfully
 {
  ret = -3;
 }
 return ret;
}

int loadBlockArray(Block *ar, int maxlen, const char *filename)
{
 int ret = 0;
 FILE *fp;
 fp = fopen(filename, "r");
 if (!fp) return 0;
 ret = fread(ar, sizeof(Block), maxlen, fp);

 if (fclose(fp) != 0)   //File not closed succesfully
 {
  ret = -ret;
 }
 return ret;
}

int addInput(int x, int y, int z, int pin)
{
 if (sortedFindBlock(inputs, n_inputs, x, y, z) >= 0)   //Block is alreadz registered
 {
  return -1;
 }
 if (n_inputs < MAXLEN)
 {
  inputs[n_inputs].X = x;
  inputs[n_inputs].Y = y;
  inputs[n_inputs].Z = z;
  inputs[n_inputs].VAL = pin;
  n_inputs++;
  qsort (inputs, n_inputs, sizeof(Block), BlockCompare);  //Sort the array for faster searching
  return 1;
 } else
 {
  printf("MCmodule: Error adding input: buffer is full (max = %d).\n", MAXLEN);
  return -2;
 }
}
int addOutput(int x, int y, int z, int pin)
{
 if (sortedFindBlock(outputs, n_outputs, x, y, z) >= 0)   //Block is alreadz registered
 {
  return -1;
 }
 if (n_outputs < MAXLEN)
 {
  outputs[n_outputs].X = x;
  outputs[n_outputs].Y = y;
  outputs[n_outputs].Z = z;
  outputs[n_outputs].VAL = pin;
  n_outputs++;
  qsort (outputs, n_outputs, sizeof(Block), BlockCompare);  //Sort the array for faster searching
  return 1;
 } else
 {
  printf("MCmodule: Error adding output: buffer is full (max = %d).\n", MAXLEN);
  return -2;
 }
}
int addSign(int x, int y, int z, int type)
{
 if (sortedFindBlock(signs, n_signs, x, y, z) >= 0)   //Block is alreadz registered
 {
  return -1;
 }
 if (n_signs < MAXLEN)
 {
  signs[n_signs].X = x;
  signs[n_signs].Y = y;
  signs[n_signs].Z = z;
  signs[n_signs].VAL = type;
  n_signs++;
  qsort (signs, n_signs, sizeof(Block), BlockCompare);  //Sort the array for faster searching
  return 1;
 } else
 {
  printf("MCmodule: Error adding sign: buffer is full (max = %d).\n", MAXLEN);
  return -2;
 }
}

int remInput(int x, int y, int z)
{
 int index = sortedFindBlock(inputs, n_inputs, x, y, z);
 if (index >= 0)    //Block found
 {
  rem_block(inputs, index, n_inputs);
  n_inputs--;
  return 1;
 } else //Block not found
 {
  printf("MCmodule: Could not remove block [%d,%d,%d] from inputs -- not found in the array!\n", x, y, z);
  return 0;
 }
}
int remOutput(int x, int y, int z)
{
 int index = sortedFindBlock(outputs, n_outputs, x, y, z);
 if (index >= 0)    //Block found
 {
  rem_block(outputs, index, n_outputs);
  n_outputs--;
  return 1;
 } else //Block not found
 {
  printf("MCmodule: Could not remove block [%d,%d,%d] from outputs -- not found in the array!\n", x, y, z);
  return 0;
 }
}
int remSign(int x, int y, int z)
{
 int index = sortedFindBlock(signs, n_signs, x, y, z);
 if (index >= 0)    //Block found
 {
  rem_block(signs, index, n_signs);
  n_signs--;
  return 1;
 } else //Block not found
 {
  printf("MCmodule: Could not remove block [%d,%d,%d] from signs -- not found in the array!\n", x, y, z);
  return 0;
 }
}

int sortedFindBlock(Block *ar, int arlen, int x, int y, int z)
{
 if (arlen == 0) return -1; //cannot be found...

 int index = arlen/2;   //start from the middle
 int range = arlen/2;
 Block bl;
 bl.X = x;
 bl.Y = y;
 bl.Z = z;
 int com_res;
 while ((com_res = BlockCompare((void*)&bl, (void*)(ar+index)) != 0))
 {
  range = index/2;
  if (range == 0)   //not found!!
  {
   return -1;
  }

  if (com_res > 0)  //ar[index] is smaller block we are looking for
  {
   index = index + range;
  } else  //ar[index] is bigger block we are looking for
  {
   index = index - range;
  }
 }
 return index;
}

int BlockCompare(const void* p1, const void* p2)
{
 Block *a = (Block*)p1;
 Block *b = (Block*)p2;
 if (a->X > b->X)
 {
  return 1;
 } else if (a->X == b->X)
 {
  if (a->Y > b->Y)
  {
   return 1;
  } else if (a->Y == b->Y)
  {
   if (a->Z > b->Z)
   {
    return 1;
   } else if (a->Z == b->Z) //The same!
   {
    return 0;
   } else   //a->Z < b->Z
   {
    return -1;
   }
  } else    //a->Y < b->Y
  {
   return -1;
  }
 } else //a->X < b->X
 {
  return -1;
 }
}

//Onlz shifts all elements, doesnt even remove the last redundant one (no need for that)
void rem_block(Block *ar, int index, int arlen)
{
 for (int i = index; i < arlen-1; i++)
 {
  ar[i].X = ar[i+1].X;
  ar[i].Y = ar[i+1].Y;
  ar[i].Z = ar[i+1].Z;
  ar[i].VAL = ar[i+1].VAL;
 }
}
