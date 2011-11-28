/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
	void screenupdate();
	bool quitRequested();
	void startup();
	void startup1();
	void switchryanon();
	void switchryanoff();
	uint16 allocatemem(uint16 paragraphs);
	void deallocatemem(uint16 segment);
	uint8 *workspace();
	uint8 *textUnder();
	void allocatework();
	void clearwork();
	void standardload();
	uint16 standardload(const char *fileName); // Returns a segment handle which needs to be freed with deallocatemem for symmetry
	void loadintotemp();
	void loadintotemp2();
	void loadintotemp3();
	void loadintotemp(const char *fileName);
	void loadintotemp2(const char *fileName);
	void loadintotemp3(const char *fileName);
	void loadtempcharset();
	void loadtempcharset(const char *fileName);
	Frame *tempCharset();
	void saveload();
	void printcurs();
	void delcurs();
	void hangoncurs(uint16 frameCount);
	void hangoncurs();
	void multidump();
	void multidump(uint16 x, uint16 y, uint8 width, uint8 height);
	void frameoutv(uint8 *dst, const uint8 *src, uint16 pitch, uint16 width, uint16 height, int16 x, int16 y);
	void frameoutnm(uint8 *dst, const uint8 *src, uint16 pitch, uint16 width, uint16 height, uint16 x, uint16 y);
	void frameoutbh(uint8 *dst, const uint8 *src, uint16 pitch, uint16 width, uint16 height, uint16 x, uint16 y);
	void frameoutfx(uint8 *dst, const uint8 *src, uint16 pitch, uint16 width, uint16 height, uint16 x, uint16 y);
	void worktoscreen();
	void workToScreenCPP();
	void multiget();
	void multiget(uint8 *dst, uint16 x, uint16 y, uint8 width, uint8 height);
	void convertkey();
	void cls();
	void printsprites();
	void quickquit();
	void readoneblock();
	void printundermon();
	void seecommandtail();
	void randomnumber();
	void quickquit2();
	uint8 getnextword(const Frame *charSet, const uint8 *string, uint8 *totalWidth, uint8 *charCount);
	void printboth(const Frame* charSet, uint16 *x, uint16 y, uint8 c, uint8 nextChar);
	void printchar();
	void printchar(const Frame* charSet, uint16 *x, uint16 y, uint8 c, uint8 nextChar, uint8 *width, uint8 *height);
	void printchar(const Frame* charSet, uint16 x, uint16 y, uint8 c, uint8 nextChar, uint8 *width, uint8 *height);
	void printdirect();
	uint8 printdirect(const uint8** string, uint16 x, uint16 *y, uint8 maxWidth, bool centered);
	uint8 printdirect(const uint8* string, uint16 x, uint16 y, uint8 maxWidth, bool centered);
	void printmessage(uint16 x, uint16 y, uint8 index, uint8 maxWidth, bool centered);
	void printmessage();
	void usetimedtext();
	void dumptimedtext();
	void setuptimedtemp();
	void setuptimedtemp(uint8 textIndex, uint8 voiceIndex, uint8 x, uint8 y, uint16 countToTimed, uint16 timeCount);
	void getundertimed();
	void putundertimed();
	uint8 printslow(const uint8 *string, uint16 x, uint16 y, uint8 maxWidth, bool centered);
	void printslow();
	void dumptextline();
	void getnumber();
	uint8 getnumber(const Frame *charSet, const uint8 *string, uint16 maxWidth, bool centered, uint16 *offset);
	uint8 kernchars(uint8 firstChar, uint8 secondChar, uint8 width);
	void oldtonames();
	void namestoold();
	void loadpalfromiff();
	void getroomdata();
	Room *getroomdata(uint8 room);
	void readheader();
	void fillspace();
	void startloading(const Room *room);
	Sprite *spritetable();
	void showframe();
	void showframe(const Frame *frameData, uint16 x, uint16 y, uint16 frameNumber, uint8 effectsFlag, uint8 *width, uint8 *height);
	void showframe(const Frame *frameData, uint16 x, uint16 y, uint16 frameNumber, uint8 effectsFlag);
	void printasprite(const Sprite *sprite);
	void width160();
	void multiput(const uint8 *src, uint16 x, uint16 y, uint8 width, uint8 height);
	void multiput();
	void eraseoldobs();
	void clearsprites();
	Sprite *makesprite(uint8 x, uint8 y, uint16 updateCallback, uint16 frameData, uint16 somethingInDi);
	void spriteupdate();
	void initman();
	void mainman(Sprite *sprite);
	void facerightway();
	void walking(Sprite *sprite);
	void autosetwalk();
	void checkdest(const RoomPaths *roomsPaths);
	void aboutturn(Sprite *sprite);
	void backobject(Sprite *sprite);
	void constant(Sprite *sprite, SetObject *objData);
	void steady(Sprite *sprite, SetObject *objData);
	void random(Sprite *sprite, SetObject *objData);
	void dodoor(Sprite *sprite, SetObject *objData, Common::Rect check);
	void doorway(Sprite *sprite, SetObject *objData);
	void widedoor(Sprite *sprite, SetObject *objData);
	void lockeddoorway(Sprite *sprite, SetObject *objData);
	void liftsprite(Sprite *sprite, SetObject *objData);
	Frame *findsource();
	void showgamereel();
	void showgamereel(ReelRoutine *routine);
	void showreelframe(Reel *reel);
	const Frame *getreelframeax(uint16 frame);
	void turnpathon(uint8 param);
	void turnpathoff(uint8 param);
	void turnpathon();
	void turnpathoff();
	void turnanypathon(uint8 param, uint8 room);
	void turnanypathoff(uint8 param, uint8 room);
	void turnanypathon();
	void turnanypathoff();
	RoomPaths *getroomspaths();
	void makebackob(SetObject *objData);
	void modifychar();
	void lockmon();
	void cancelch0();
	void cancelch1();
	void plotreel();
	Reel *getreelstart();
	void dealwithspecial(uint8 firstParam, uint8 secondParam);
	void zoom();
	void crosshair();
	void showrain();
	void deltextline();
	void commandonly();
	void commandonly(uint8 command);
	void doblocks();
	void checkifperson();
	bool checkifperson(uint8 x, uint8 y);
	void checkiffree();
	bool checkiffree(uint8 x, uint8 y);
	void checkifex();
	bool checkifex(uint8 x, uint8 y);
	const uint8 *findobname(uint8 type, uint8 index);
	void copyname();
	void copyname(uint8 type, uint8 index, uint8 *dst);
	void commandwithob();
	void commandwithob(uint8 command, uint8 type, uint8 index);
	void showpanel();
	void updatepeople();
	void madmantext();
	void madmode();
	void movemap(uint8 param);
	bool addalong(const uint8 *mapFlags);
	bool addlength(const uint8 *mapFlags);
	void getdimension();
	void getdimension(uint8 *mapXstart, uint8 *mapYstart, uint8 *mapXsize, uint8 *mapYsize);
	void getmapad();
	void calcmapad();
	uint8 getmapad(const uint8 *setData);
	uint8 getxad(const uint8 *setData, uint8 *result);
	uint8 getyad(const uint8 *setData, uint8 *result);
	void calcfrframe();
	void calcfrframe(uint8* width, uint8* height);
	void finalframe();
	void finalframe(uint16 *x, uint16 *y);
	void showallobs();
	void blocknametext();
	void walktotext();
	void personnametext();
	void findxyfrompath();
	void findormake();
	void findormake(uint8 index, uint8 value, uint8 type);
	DynObject *getfreead(uint8 index);
	DynObject *getexad(uint8 index);
	DynObject *geteitheradCPP();
	SetObject *getsetad(uint8 index);
	void *getanyad(uint8 *value1, uint8 *value2);
	void *getanyaddir(uint8 index, uint8 flag);
	void setallchanges();
	void dochange(uint8 index, uint8 value, uint8 type);
	void deletetaken();
	bool isCD();
	void placesetobject();
	void placesetobject(uint8 index);
	void removesetobject();
	void removesetobject(uint8 index);
	void showallfree();
	void showallex();
	bool finishedwalkingCPP();
	void finishedwalking();
	void checkone();
	void checkone(uint8 x, uint8 y, uint8 *flag, uint8 *flagEx, uint8 *type, uint8 *flagX, uint8 *flagY);
	void getflagunderp();
	void getflagunderp(uint8 *flag, uint8 *flagEx);
	void walkandexamine();
	void obname();
	void obname(uint8 command, uint8 commandType);
	void delpointer();
	void showblink();
	void dumpblink();
	void dumppointer();
	void showpointer();
	void animpointer();
	void checkcoords();
	void checkcoords(const RectWithCallback *rectWithCallbacks);
	void readmouse();
	uint16 readMouseState();
	uint16 waitframes();
	void drawflags();
	void addtopeoplelist();
	void addtopeoplelist(ReelRoutine *routine);
	void getexpos();
	void paneltomap();
	void maptopanel();
	void dumpmap();
	void obpicture();
	void transferinv();
	void obicons();
	void compare();
	bool compare(uint8 index, uint8 flag, const char id[4]);
	bool pixelcheckset(const ObjPos *pos, uint8 x, uint8 y);
	bool isitdescribed(const ObjPos *objPos);
	void checkifset();
	bool checkifset(uint8 x, uint8 y);
	void checkifpathison();
	bool checkifpathison(uint8 index);
	void isitworn();
	bool isitworn(const DynObject *object);
	void wornerror();
	void makeworn();
	void makeworn(DynObject *object);
	void obtoinv();
	void obtoinv(uint8 index, uint8 flag, uint16 x, uint16 y);
	void showryanpage();
	void findallryan();
	void findallryan(uint8 *inv);
	void fillryan();
	void useroutine();
	void hangon();
	void hangon(uint16 frameCount);
	void hangonw();
	void hangonw(uint16 frameCount);
	void hangonp();
	void hangonp(uint16 count);
	void showicon();
	uint8 findnextcolon(uint8 **string);
	void findnextcolon();
	uint8 *getobtextstartCPP();
	void usetext(const uint8 *string);
	void usetext();
	void getblockofpixel();
	uint8 getblockofpixel(uint8 x, uint8 y);
	void bresenhams();
	void examineobtext();
	void sortoutmap();
	void showcity();
	uint16 getpersframe(uint8 index);
	void convicons();
	void examineob(bool examineAgain = true);
	void showwatch();
	void dumpwatch();
	void showtime();
	void roomname();
	void transfertext();
	void initrain();
	Rain *splitintolines(uint8 x, uint8 y, Rain *rain);
	uint8 *mainPalette();
	uint8 *startPalette();
	uint8 *endPalette();
	void clearstartpal();
	void clearendpal();
	void paltostartpal();
	void endpaltostart();
	void startpaltoend();
	void paltoendpal();
	void fadecalculation();
	void watchcount();
	void zoomicon();
	void loadroom();
	void getundermenu();
	void putundermenu();
	void textformonk();
	void textforend();
	void readsetdata();
	void loadroomssample();
	void fadeupyellows();
	void fadeupmonfirst();
	void printlogo();
	void usemon();
	void scrollmonitor();
	void showcurrentfile();
	void input();
	void monprint();
	const char *monprint(const char *string);
	Frame *tempGraphics();
	Frame *tempGraphics2();
	Frame *tempGraphics3();
	void accesslighton();
	void accesslightoff();
	void randomaccess(uint16 count);
	void randomaccess();
	void monmessage(uint8 index);
	void monmessage();
	void neterror();
	void turnonpower();
	void powerlighton();
	void powerlightoff();
	void playchannel0();
	void playchannel0(uint8 index, uint8 repeat);
	void playchannel1();
	void playchannel1(uint8 index);
	void showmainops();
	void showdiscops();
	void createpanel();
	void createpanel2();
	void findroominloc();
	void reelsonscreen();
	void reconstruct();
	void look();
	void autolook();
	void dolook();
	void usetempcharset();
	void usecharset1();
	void getbackfromob();
	void showfirstuse();
	void showseconduse();
	void actualsave();
	void actualload();
	void loadposition(unsigned int slot);
	void saveposition(unsigned int slot, const uint8 *descbuf);
	void openforsave(unsigned int slot);
	void openforload(unsigned int slot);
	uint16 allocateAndLoad(unsigned int size);
	void clearAndLoad(uint16 seg, uint8 c, unsigned int size, unsigned int maxSize);
	void loadRoomData(const Room* room, bool skipDat);
	void restoreall();
	void restorereels();
	void viewfolder();
	void showfolder();
	void showleftpage();
	void showrightpage();

