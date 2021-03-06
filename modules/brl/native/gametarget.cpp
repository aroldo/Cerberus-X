
//***** game.h *****

struct BBGameEvent{
	enum{
		None=0,
		KeyDown=1,KeyUp=2,KeyChar=3,
		MouseDown=4,MouseUp=5,MouseMove=6,
		TouchDown=7,TouchUp=8,TouchMove=9,
		MotionAccel=10
	};
};

class BBGameDelegate : public Object{
public:
	virtual void StartGame(){}
	virtual void SuspendGame(){}
	virtual void ResumeGame(){}
	virtual void UpdateGame(){}
	virtual void RenderGame(){}
	virtual void KeyEvent( int event,int data ){}
	virtual void MouseEvent( int event,int data,Float x,Float y, Float z ){}
	virtual void TouchEvent( int event,int data,Float x,Float y ){}
	virtual void MotionEvent( int event,int data,Float x,Float y,Float z ){}
	virtual void DiscardGraphics(){}
	virtual void FileDropEvent(String filename){}
};

struct BBDisplayMode : public Object{
	int width;
	int height;
	int depth;
	int hertz;
	int flags;
	BBDisplayMode( int width=0,int height=0,int depth=0,int hertz=0,int flags=0 ):width(width),height(height),depth(depth),hertz(hertz),flags(flags){}
};

class BBGame{
public:
	BBGame();
	virtual ~BBGame(){}
	
	// ***** Extern *****
	static BBGame *Game(){ return _game; }
	
	virtual void SetDelegate( BBGameDelegate *delegate );
	virtual BBGameDelegate *Delegate(){ return _delegate; }
	
	virtual void SetKeyboardEnabled( bool enabled );
	virtual bool KeyboardEnabled();
	
	virtual void SetUpdateRate( int updateRate );
	virtual int UpdateRate();
	
	virtual bool Started(){ return _started; }
	virtual bool Suspended(){ return _suspended; }
	
	virtual int Millisecs();
	virtual void GetDate( Array<int> date );
	virtual int SaveState( String state );
	virtual String LoadState();
	virtual String LoadString( String path );
	virtual int CountJoysticks( bool update );
	virtual bool PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons );
    virtual String GetJoyVendor (String vendor);
    virtual Float JoyBattLevel(Float level);
	virtual void OpenUrl( String url );
	virtual void SetMouseVisible( bool visible );
	virtual void SetMousePos( double xpos, double ypos );
	virtual void SetClipboard( String _text );
	virtual String GetClipboard();
	
	virtual int GetDeviceWidth(){ return 0; }
	virtual int GetDeviceHeight(){ return 0; }
	virtual int GetDeviceWindowWidth(){ return this->GetDeviceWidth(); }
	virtual int GetDeviceWindowHeight(){ return this->GetDeviceHeight(); }
	virtual void SetDeviceWindow( int width,int height,int flags ){}
	virtual void SetDeviceWindowIcon( String _path ){}
	virtual void SetDeviceWindowPosition( int _x, int _y ){}
	virtual void SetDeviceWindowSize( int _width, int _height ){}
	virtual void SetDeviceWindowSizeLimits( int _minWidth, int _minHeight, int _maxWidth, int _maxHeight ){}
	virtual void SetDeviceWindowTitle( String _title ){}
	virtual Array<BBDisplayMode*> GetDisplayModes(){ return Array<BBDisplayMode*>(); }
	virtual BBDisplayMode *GetDesktopMode(){ return 0; }
	virtual void SetSwapInterval( int interval ){}

	// ***** Native *****
	virtual String PathToFilePath( String path );
	virtual FILE *OpenFile( String path,String mode );
	virtual unsigned char *LoadData( String path,int *plength );
	virtual unsigned char *LoadImageData( String path,int *width,int *height,int *depth ){ return 0; }
	virtual unsigned char *LoadAudioData( String path,int *length,int *channels,int *format,int *hertz ){ return 0; }
	
	//***** Internal *****
	virtual void Die( ThrowableObject *ex );
	virtual void gc_collect();
	virtual void StartGame();
	virtual void SuspendGame();
	virtual void ResumeGame();
	virtual void UpdateGame();
	virtual void RenderGame();
	virtual void KeyEvent( int ev,int data );
	virtual void MouseEvent( int ev,int data,float x,float y, float z );
	virtual void TouchEvent( int ev,int data,float x,float y );
	virtual void MotionEvent( int ev,int data,float x,float y,float z );
	virtual void DiscardGraphics();
	virtual void FileDropEvent(String filename);
	
protected:

	static BBGame *_game;

	BBGameDelegate *_delegate;
    GCController *controller;   // Code developed and added by Aroldo Carvalho for iOS Game Controller support on Dec 27, 2020.
    
	bool _keyboardEnabled;
	int _updateRate;
	bool _started;
	bool _suspended;
};

//***** game.cpp *****

BBGame *BBGame::_game;

BBGame::BBGame():
_delegate( 0 ),
_keyboardEnabled( false ),
_updateRate( 0 ),
_started( false ),
_suspended( false ){
	_game=this;
}

void BBGame::SetDelegate( BBGameDelegate *delegate ){
	_delegate=delegate;
}

void BBGame::SetKeyboardEnabled( bool enabled ){
	_keyboardEnabled=enabled;
}

bool BBGame::KeyboardEnabled(){
	return _keyboardEnabled;
}

void BBGame::SetUpdateRate( int updateRate ){
	_updateRate=updateRate;
}

int BBGame::UpdateRate(){
	return _updateRate;
}

int BBGame::Millisecs(){
	return 0;
}

void BBGame::GetDate( Array<int> date ){
	int n=date.Length();
	if( n>0 ){
		time_t t=time( 0 );
		
#if _MSC_VER
		struct tm tii;
		struct tm *ti=&tii;
		localtime_s( ti,&t );
#else
		struct tm *ti=localtime( &t );
#endif

		date[0]=ti->tm_year+1900;
		if( n>1 ){ 
			date[1]=ti->tm_mon+1;
			if( n>2 ){
				date[2]=ti->tm_mday;
				if( n>3 ){
					date[3]=ti->tm_hour;
					if( n>4 ){
						date[4]=ti->tm_min;
						if( n>5 ){
							date[5]=ti->tm_sec;
							if( n>6 ){
								date[6]=0;
							}
						}
					}
				}
			}
		}
	}
}

int BBGame::SaveState( String state ){
	if( FILE *f=OpenFile( "./.cerberusstate","wb" ) ){
		bool ok=state.Save( f );
		fclose( f );
		return ok ? 0 : -2;
	}
	return -1;
}

String BBGame::LoadState(){
	if( FILE *f=OpenFile( "./.cerberusstate","rb" ) ){
		String str=String::Load( f );
		fclose( f );
		return str;
	}
	return "";
}

String BBGame::LoadString( String path ){
	if( FILE *fp=OpenFile( path,"rb" ) ){
		String str=String::Load( fp );
		fclose( fp );
		return str;
	}
	return "";
}

// Implementation of Apple GCController by Aroldo Carvalho on Dec 26th, 2020. APC.
// Because CerberusX polls data I implemented a polling system and the notifications system.
// CountJoysticks returns the number of connected game controllers up to 4.
int BBGame::CountJoysticks( bool update ){
    if(update){
        return int([[GCController controllers] count]);
    }
    return 0;
}

Float BBGame::JoyBattLevel(Float level){
    if (controller){
        GCDeviceBattery *db = controller.battery;
        return db.batteryLevel;
    }
    return 0.0;
}
String BBGame::GetJoyVendor(String vendor){
    if (controller){
            return controller.productCategory;
    }
    return "";
}

// PollJoystick returns the digital buttons and analog buttons. APC.
// Since Cerberus loops on all 4 possible game controllers connected to the iOS Device.
bool BBGame::PollJoystick( int port,Array<Float> joyx,Array<Float> joyy,Array<Float> joyz,Array<bool> buttons ){
    int ccount = int( [[GCController controllers] count]);
    if (ccount <=0){
        //printf( "*** No Controllers:\n" );
        return false;
    }
    else{
        //printf("%s \n", [(controller.productCategory) UTF8String]);
        if (port < ccount){ // read only the data of connected game controllers.
            controller = [GCController controllers][port];
            // XBox A,B,X,Y or PLay Station SQUARE, TRIANGLE, X and CIRCLE buttons
            buttons[0] = controller.extendedGamepad.buttonA.value;
            buttons[1] = controller.extendedGamepad.buttonB.value;
            buttons[2] = controller.extendedGamepad.buttonX.value;
            buttons[3] = controller.extendedGamepad.buttonY.value;
            // Left and Right Shoulder Buttons
            buttons[4] = controller.extendedGamepad.leftShoulder.value;
            buttons[5] = controller.extendedGamepad.rightShoulder.value;
            // XBox BACK or Play Station SHARE buttons
            buttons[6] = controller.extendedGamepad.buttonOptions.value;
            buttons[7] = controller.extendedGamepad.buttonMenu.value;
            // Digital Pad LEFT, UP, RIGHT and DOWN buttons
            buttons[8] = controller.extendedGamepad.dpad.left.value;
            buttons[9] = controller.extendedGamepad.dpad.up.value;
            buttons[10] = controller.extendedGamepad.dpad.right.value;
            buttons[11] = controller.extendedGamepad.dpad.down.value;
            // Left and Right Stick Buttons
            buttons[12] = controller.extendedGamepad.leftThumbstickButton.value;
            buttons[13] = controller.extendedGamepad.rightThumbstickButton.value;
            //XBox XBOX and Play Station PS buttons - Menu
            buttons[14] = controller.extendedGamepad.buttonHome.value;
            //Left Analog Joystick and Trigger
            joyx[0] = controller.extendedGamepad.leftThumbstick.xAxis.value;
            joyy[0] = controller.extendedGamepad.leftThumbstick.yAxis.value;
            joyz[0] = controller.extendedGamepad.leftTrigger.value;
            //Right Analog Joystick and Trigger
            joyx[1] = controller.extendedGamepad.rightThumbstick.xAxis.value;
            joyy[1] = controller.extendedGamepad.rightThumbstick.yAxis.value;
            joyz[1] = controller.extendedGamepad.rightTrigger.value;
            
            
            return true;
        }
    }
    return false;
}

void BBGame::OpenUrl( String url ){
}

void BBGame::SetMouseVisible( bool visible ){
}

void BBGame::SetMousePos( double xpos, double ypos ){
}

void BBGame::SetClipboard( String _text ){
}

String BBGame::GetClipboard(){
	return "";
}

//***** C++ Game *****

String BBGame::PathToFilePath( String path ){
	return path;
}

FILE *BBGame::OpenFile( String path,String mode ){
	path=PathToFilePath( path );
	if( path=="" ) return 0;
	
#if __cplusplus_winrt
	path=path.Replace( "/","\\" );
	FILE *f;
	if( _wfopen_s( &f,path.ToCString<wchar_t>(),mode.ToCString<wchar_t>() ) ) return 0;
	return f;
#elif _WIN32
	return _wfopen( path.ToCString<wchar_t>(),mode.ToCString<wchar_t>() );
#else
	return fopen( path.ToCString<char>(),mode.ToCString<char>() );
#endif
}

unsigned char *BBGame::LoadData( String path,int *plength ){

	FILE *f=OpenFile( path,"rb" );
	if( !f ) return 0;

	const int BUF_SZ=4096;
	std::vector<void*> tmps;
	int length=0;
	
	for(;;){
		void *p=malloc( BUF_SZ );
		int n=fread( p,1,BUF_SZ,f );
		tmps.push_back( p );
		length+=n;
		if( n!=BUF_SZ ) break;
	}
	fclose( f );
	
	unsigned char *data=(unsigned char*)malloc( length );
	unsigned char *p=data;
	
	int sz=length;
	for( int i=0;i<tmps.size();++i ){
		int n=sz>BUF_SZ ? BUF_SZ : sz;
		memcpy( p,tmps[i],n );
		free( tmps[i] );
		sz-=n;
		p+=n;
	}
	
	*plength=length;
	
	gc_ext_malloced( length );
	
	return data;
}

//***** INTERNAL *****

void BBGame::Die( ThrowableObject *ex ){
	bbPrint( "Cerberus Runtime Error : Uncaught Cerberus Exception" );
#ifndef NDEBUG
	bbPrint( ex->stackTrace );
#endif
	exit( -1 );
}

void BBGame::gc_collect(){
	gc_mark( _delegate );
	::gc_collect();
}

void BBGame::StartGame(){

	if( _started ) return;
	_started=true;
	
	try{
		_delegate->StartGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::SuspendGame(){

	if( !_started || _suspended ) return;
	_suspended=true;
	
	try{
		_delegate->SuspendGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::ResumeGame(){

	if( !_started || !_suspended ) return;
	_suspended=false;
	
	try{
		_delegate->ResumeGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::UpdateGame(){

	if( !_started || _suspended ) return;
	
	try{
		_delegate->UpdateGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::RenderGame(){

	if( !_started ) return;
	
	try{
		_delegate->RenderGame();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::FileDropEvent( String filename ){

	if( !_started ) return;
	
	try{
		_delegate->FileDropEvent( filename );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::KeyEvent( int ev,int data ){

	if( !_started ) return;
	
	try{
		_delegate->KeyEvent( ev,data );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::MouseEvent( int ev,int data,float x,float y, float z ){

	if( !_started ) return;
	
	try{
		_delegate->MouseEvent( ev,data,x,y,z );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::TouchEvent( int ev,int data,float x,float y ){

	if( !_started ) return;
	
	try{
		_delegate->TouchEvent( ev,data,x,y );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::MotionEvent( int ev,int data,float x,float y,float z ){

	if( !_started ) return;
	
	try{
		_delegate->MotionEvent( ev,data,x,y,z );
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}

void BBGame::DiscardGraphics(){

	if( !_started ) return;
	
	try{
		_delegate->DiscardGraphics();
	}catch( ThrowableObject *ex ){
		Die( ex );
	}
	gc_collect();
}
