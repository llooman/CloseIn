#include "CloseIn.h"

class EEParms:  public EEUtil
{
public:
	volatile int boiler_maxTemp 	= 56;       // referentie temp
	volatile int ssr_maxTemp 		= 70;      // aan uit hysteresis
	long chk1  = 0x00010203;

	EEParms(){}
	virtual ~EEParms(){}

    void setup( ) //
    {
		if( readLong(offsetof(EEParms, chk1)) == chk1  )
    	{
    		readAll();
    		setSSRTemp(   readInt(offsetof(EEParms, ssr_maxTemp)));
    		setBoilerTemp(readInt(offsetof(EEParms, boiler_maxTemp)));
    		changed = false;
			#ifdef DEBUG
				Serial.println(F("EEProm.read"));
			#endif
    	}
		else
		{
			bootCount=0;
		}
    }

	void loop()
	{
		if(changed)
		{
			#ifdef DEBUG
				Serial.println(F("Parms.loop.changed"));
			#endif
			write(offsetof(EEParms, boiler_maxTemp), boiler_maxTemp);
			write(offsetof(EEParms, ssr_maxTemp), ssr_maxTemp);
			write(offsetof(EEParms, chk1), chk1);
			EEUtil::writeAll();
			changed = false;
		}
	}

	void setBoilerTemp( long newVal)
    {
    	boiler_maxTemp = (int) newVal;
		if(boiler_maxTemp < 10) boiler_maxTemp = 10;
		if(boiler_maxTemp > 60) boiler_maxTemp = 60;
		changed = true;
    }
    void setSSRTemp( long newVal)
    {
    	ssr_maxTemp = (int) newVal;
		if(ssr_maxTemp < 20) ssr_maxTemp = 20;
		if(ssr_maxTemp > 80) ssr_maxTemp = 80;
		changed = true;
    }

};
EEParms eeParms;


#ifdef SPI_CONSOLE
	ISR (SPI_STC_vect){	console.handleInterupt();}
#endif
#ifdef	TWI_PARENT
	ISR(TWI_vect){ parentNode.tw_int(); }
#endif
#ifdef SPI_PARENT
	ISR (SPI_STC_vect){parentNode.handleInterupt();}
#endif
 
void localSetVal(int id, long val)
{
	switch(id )
	{
		case WATER_TEMP_MAX_ID:
			eeParms.setBoilerTemp(val);
			// dutyCycleTimer = 0;
			break;
		case SSR_TEMP_MAX_ID:
			eeParms.setSSRTemp(val);
			// dutyCycleTimer = 0;
			break;

	default:
		eeParms.setVal( id,  val);
		break;
	}
}

void nextUpload(int id){
	switch( id ){
 
		// case 50: myTimers.nextTimer(TIMER_UPLOAD_LED);		break;
	}
}

int upload(int id)
{

	nextUpload(id);
	int ret = 0;

	switch( id )
	{
	case 8:
		upload(id, JL_VERSION );   
		break;

	case WATER_TEMP_ID: 	tempWater.upload();  break;

	case WATER_TEMP_MAX_ID:	upload(id, eeParms.boiler_maxTemp);  	break;
	case SSR_TEMP_MAX_ID: 	upload(id, eeParms.ssr_maxTemp);  	break;

	#ifdef S202s02
		case SSR_TEMP_ID: tempSSR.upload();  break;
	#endif 

 
	case SSR_ID:
		boiler.uploadIsRunning();
		break;
	case SSR_ID+1:
		boiler.uploadState() ;  
		break;	
	case SSR_ID+2:
		boiler.uploadManual();
		break;


	case 50: // LED: LOW = on, HIGH = off 
		upload(id, !digitalRead(LED_BUILTIN) );
		break;

	default:
		if( 1==2
		 ||	parentNode.upload(id)>0
		 ||	eeParms.upload(id)>0
		){}
		break;
	}
	return ret;
}


int upload(int id, long val) { return upload(id, val, millis()); }
int upload(int id, long val, unsigned long timeStamp)
{
	#ifdef DEBUG
		Serial.print("upload- ");Serial.println(id);
	#endif
	return parentNode.txUpload(id, val, timeStamp);
}
int uploadError(int id, long val)
{
	return parentNode.txError(id, val);
}

int handleParentReq( RxItem *rxItem)  // cmd, to, parm1, parm2
{
	#ifdef DEBUG
		parentNode.debug("Prnt<", rxItem);
	#endif

	if( rxItem->data.msg.node==2
	 || rxItem->data.msg.node==0
	 || rxItem->data.msg.node==parentNode.nodeId )
	{
		return localRequest(rxItem);
	}

	if(parentNode.nodeId==0)
	{
		#ifdef DEBUG
			parentNode.debug("skip", rxItem);
		#endif

		return 0;
	}

	#ifdef DEBUG
		parentNode.debug("forward", rxItem);
	#endif

	#ifndef SKIP_CHILD_NODES
	//	return childNodes.putBuf( req );
	#endif
}


int localRequest(RxItem *rxItem)
{
	#ifdef DEBUG
		parentNode.debug("local", rxItem);
	#endif

	int ret=0;
	int subCmd;
	int val;

	switch (  rxItem->data.msg.cmd)
	{
	case 'L':
		switch (rxItem->data.msg.id)
		{
		case 1:
			trace(); 
			break;
		}
		break;

	case 'l':
	    subCmd = rxItem->data.msg.val & 0x000000ff ;
    	val = rxItem->data.msg.val >> 8;
		switch (rxItem->data.msg.id)
		{
		case 1:
			parentNode.localCmd(subCmd, val);
			break;
 		// case 2:
		// 	mhz19b.localCmd(subCmd, val);
		// 	break;
		}
		break;


	case 's':
	case 'S':
		localSetVal(rxItem->data.msg.id, rxItem->data.msg.val);
		upload(rxItem->data.msg.id);
		break;
	case 'r':
	case 'R':
		upload(rxItem->data.msg.id);
		break;

	case 'b':
		eeParms.resetBootCount();
 		upload(3);
		break;

	case 'B':
		wdt_enable(WDTO_15MS);
		while(true){
			delay(500);
			asm volatile ("  jmp 0");
		}
		break;
	default:
		eeParms.handleRequest(rxItem);
		break;
	}

	return ret;
}


void setup()   // TODO
{
// *******************************
	wdt_reset();
	wdt_disable();
	wdt_enable(WDTO_8S);
	// pinMode(LED_BUILTIN, OUTPUT);

	Serial.begin(115200);

	#ifdef DEBUG
		Serial.print("DEBUG ");
	#endif

	#ifdef S202s02
		Serial.println("S202s02...");
	#else
		Serial.println("SSR-10DA...");
	#endif

	tempSSR.onUpload(upload);
	tempSSR.onError(uploadError);
	tempSSR.sensorFrequency_s = 7;
	tempWater.onUpload(upload);
	tempWater.onError(uploadError);
	tempWater.sensorFrequency_s = 15;

	eeParms.onUpload(upload);
	eeParms.setup();

	int nodeId = NODE_ID;

	parentNode.onReceive( handleParentReq);
	parentNode.onError(uploadError);
	parentNode.onUpload(upload);
	parentNode.nodeId = nodeId;
	parentNode.isParent = true;


	#ifdef TWI_PARENT
		parentNode.begin();
		//parentNode.txBufAutoCommit = true;
	#endif

	#ifdef NETW_PARENT_SPI
		bool isSPIMaster = false;
		parentNode.setup( SPI_PIN, isSPIMaster);
		parentNode.isParent = true;
	#endif

	boiler.onCheck(evaluatePerc);
	boiler.onUpload(upload, SSR_ID);
	boiler.setDebounce_s(0, 15);
	// boiler.sayHello = true;
	boiler.dutyCycleMode = true;
	boiler.numberOfStates = 10;
	// boiler.uploadDutyCycle = true;
	#ifdef DEBUG
		boiler.test(true);
	#endif


	myTimers.nextTimer(TIMER_TRACE, 2);
	myTimers.nextTimer(TIMER_UPLOAD_ON_BOOT, 0);

	// myTimers.nextTimer(TIMER_TEST, 50);

 
	wdt_reset();
	wdt_enable(WDTO_8S);
	wdt_reset();
	delay(0);

}  // end setup

int evaluatePerc(bool on, bool isRunning){

	// return 0; // 0=off, 1=on, -1=nop, >1 = %

	unsigned long ssrTempAge_ms = millis() - tempSSR.timeStamp;

	// if( ssrTempAge_ms > 10000 ) Serial.println("ssrTempAge");

	if( tempWater.errorCnt > 2
	 || tempSSR.errorCnt > 0
	 || tempWater.temp > (eeParms.boiler_maxTemp * 100)
	 || tempSSR.temp > (eeParms.ssr_maxTemp * 100 )
	 || ssrTempAge_ms > 10000
	){
		#ifdef DEBUG
			if(on)Serial.print(" Off");					
		#endif
		// boiler.force = true;
		boiler.resetManual();
		return 0;
	}

	// omgekeerd evenredig aan de laatste 20 graden below max ssr temp
	// bij max 80    delta  %
	// >= 80: 0%      0
	//    70: 50%     10	50
	// <= 60: 100%    20	100

	int heatingPerc = 5 * (eeParms.ssr_maxTemp - tempSSR.getGraden()); 
	return heatingPerc;
}


void loop()  // TODO
{
	wdt_reset();

	boiler.loop();
	tempSSR.loop();
	tempWater.loop();
	eeParms.loop();
	// parentNode.loopSerial();
	parentNode.loop();

	if( parentNode.isReady() 
	 && ! parentNode.isTxFull()
	){
		if( myTimers.isTime(TIMER_UPLOAD_ON_BOOT)
		){
			switch( uploadOnBootCount )
			{
			case 1:
			    if(millis()<60000) upload(1);
				break;    // boottimerr

			case 3: upload(2); break;
			case 4: upload(3); break;
			case 5: upload(8); break;	 
			case 6: upload(50); break;

			case 7: upload(WATER_TEMP_ID); break;  	// temp
			case 8: upload(SSR_TEMP_ID); break;  	// temp
			case 9: upload(WATER_TEMP_MAX_ID); break;  	// temp
			case 10: upload(SSR_TEMP_MAX_ID); break;  	// temp

			// case 11: upload(SSR_ID); break;	// max temp 
			// case 12: upload(SSR_ID+1); break;	// max ssr temp 
 
			case 30: myTimers.timerOff(TIMER_UPLOAD_ON_BOOT); break;			
			}

			uploadOnBootCount++;
			myTimers.nextTimerMillis(TIMER_UPLOAD_ON_BOOT, TWI_SEND_ERROR_INTERVAL);
		}
	}

 
	#ifdef DEBUG		
		if( myTimers.isTime(TIMER_TRACE)
		){ trace();}
		 
	#endif

	wdt_reset();
	delay(0);
} // end loop


void trace( )
{
	myTimers.nextTimer(TIMER_TRACE, TRACE_SEC);

	Serial.print("@ ");
	Serial.print(millis() / 1000);
	Serial.print(": ");
	// Serial.print(F(", heating=")); Serial.print(heatingPerc);
	Serial.print(F(" maxTemp=")); Serial.print(eeParms.boiler_maxTemp);
	Serial.print(F(", maxSsr=")); Serial.print(eeParms.ssr_maxTemp);
	Serial.print(F(", heat%=")); Serial.print((eeParms.ssr_maxTemp - tempSSR.getGraden()) * 5);

	Serial.println();

	tempWater.trace("water");

	tempSSR.trace("ssr");

	// parentNode.trace("pNd");

	boiler.trace("boiler");

	Serial.flush();
}