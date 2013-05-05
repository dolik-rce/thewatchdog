topic "Client (reference implementation)";
[2 $$0,0#00000000000000000000000000000000:Default]
[l288;i1120;a17;O9;~~~.1408;2 $$1,0#10431211400427159095818037425705:param]
[a83;*R6 $$2,5#31310162474203024125188417583966:caption]
[H4;b83;*4 $$3,5#07864147445237544204411237157677:title]
[i288;O9;C2 $$4,6#40027414424643823182269349404212:item]
[b42;a42;2 $$5,5#45413000475342174754091244180557:text]
[l288;b17;a17;2 $$6,6#27521748481378242620020725143825:desc]
[l321;C@5;1 $$7,7#20902679421464641399138805415013:code]
[b2503;2 $$8,0#65142375456100023862071332075487:separator]
[*@(0.0.255)2 $$9,0#83433469410354161042741608181528:base]
[C2 $$10,0#37138531426314131251341829483380:class]
[l288;a17;*1 $$11,11#70004532496200323422659154056402:requirement]
[i417;b42;a42;O9;~~~.416;2 $$12,12#10566046415157235020018451313112:tparam]
[b167;C2 $$13,13#92430459443460461911108080531343:item1]
[i288;a42;O9;C2 $$14,14#77422149456609303542238260500223:item2]
[*@2$(0.128.128)2 $$15,15#34511555403152284025741354420178:NewsDate]
[l321;*C$7;2 $$16,16#03451589433145915344929335295360:result]
[l321;b83;a83;*C$7;2 $$17,17#07531550463529505371228428965313:result`-line]
[l160;*C+117 $$18,5#88603949442205825958800053222425:package`-title]
[2 $$19,0#53580023442335529039900623488521:gap]
[C2 $$20,20#70211524482531209251820423858195:class`-nested]
[b50;2 $$21,21#03324558446220344731010354752573:Par]
[{_}%EN-US 
[s2; Client (reference implementation)&]
[s0; The Watchdog/Client package contains reference implementation 
of client communicating with Watchdog/Server. It is of course 
possible to write your own client using the documented [^topic`:`/`/Watchdog`/srcdoc`/API`$en`-us^ A
PI] or call the methods directly from any application.&]
[s0; &]
[s0; The reference implementation is a simple command`-line tool 
designed to be used either as standalone tool that runs given 
command for each commit or as a simple wrapper around each of 
the API methods for easy usage in scripts.&]
[s0; &]
[s3; Usage&]
[s0; &]
[s9; Client `-`-help `| `-h&]
[s6; Prints usage information.&]
[s0; &]
[s9; Client `-`-get `| `-g `[[@3 max`_age]`]&]
[s6; Returns a list of not yet built revisions, optionally limited 
to [*@3 max`_age] days in the past.&]
[s0; &]
[s9; Client `-`-accept `| `-a [@3 revision] `[[@3 start`_time]`]&]
[s6; Notifies server about building [*@3 revision]. The optional [*@3 start`_time] 
parameter specifies when did the client started the job, defaults 
to current time.&]
[s0; &]
[s9; Client `-`-submit `| `-s [@3 revision result duration output`_file] 
`[[@3 start`_time ]`[[@3 end`_time]`]`]&]
[s6; Sends results for [*@3 revision] to the server. [*@3 Result] is 
a number from the enum in Watchdog.h, [*@3 duration] is time it 
took to perform the job in seconds and [*@3 output`_file] is a 
file with jobs output. The optional [*@3 start`_time] parameter 
specifies when did the client started the job, defaults to current 
time minus [*@3 duration] seconds. The second optional parameter, 
[*@3 end`_time], specifies when the job ended, default is current 
time.&]
[s0; &]
[s9; Client `-`-command `| `-c [@3 command] `[[@3 max`_age]`]&]
[s6; Gets and accepts work automatically, if there is a new commit 
then [*@3 command] is performed (with any `'[/ `@revision]`' string 
substituted by actual revision number) and then the result and 
output of the script are sent to the server. Optional second 
parameter can be used to restrict the execution to commits [*@3 max`_age] 
days old.&]
[s0; &]
[s0; Note: The start`_time and end`_time parameters should be given 
as int64 representation of U`+`+ Time (see [^topic`:`/`/Core`/src`/DateTime`$en`-us`#Time`:`:Get`(`)const^ U
pp`::Time`::Get()]). Current implementation ignores the timezones, 
so all the times will be interpreted in the timezone set on the 
server.&]
[s0; &]
[s3; Configuration file&]
[s0; &]
[s5; The reference implementation of client reads a file with the 
same name as executable `+`".ini`" suffix in the same directory 
or at path given in [/ UPP`_MAIN`_`_] environment variable (if 
defined). Recognized options are following:&]
[s0; &]
[s9; host `[default: http://localhost:8001/api`]&]
[s6; Watchdog API URL.&]
[s0; &]
[s9; client`_id&]
[s6; Watchdog client ID.&]
[s0; &]
[s9; password&]
[s6; Watchdog client password.&]
[s0; &]
[s9; max`_age `[default: `-1`]&]
[s6; Only commits this many days old can be build (negative value 
means any age). Can be overriden on command`-line.&]
[s0; &]
[s9; session`_cookie `[default: `_`_watchdog`_cookie`_`_`]&]
[s6; Skylark session cookie name. Must be set same as in the Server 
configuration file.&]
[s0; ]]