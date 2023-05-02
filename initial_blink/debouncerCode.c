// this struct holds the data necessary to 
//  debounce a button
struct button_debounce
{
	int gpio_num; // the pin of the button
	int lastState;
	int lastSteadyState;
	int64_t lastTime;
};


int checkBottonPressed(struct button_debounce* );

// int lastState = 0; 
// int lastSteadyState = 0;
// int64_t lastTime = 0;
// data for software debounce
// struct button_debounce switchProgramm_IO = {PIN_SWITCH_PROGS, lastState, lastSteadyState, lastTime};



/* CURRENTLY NOT NEEDED
because bottons change their state during the first milliseconds
we need a way to determine if a botton is really pressed

int gpio_num 		: number of the checked pin
int lastState 		: last state, either 0 or 1
int64_t lastTime	: last time since state is 1

returns 0 == not pressed
returns 1 == pressed
*/
int checkBottonPressed(struct button_debounce* data){

	int64_t curTime = esp_timer_get_time();

	// threshold is in mu-sec 
	int64_t threshold = 1000;
	int curState = gpio_get_level(data->gpio_num);

	int buttonState = 0;

	if(curState != data->lastState){
		data->lastTime = curTime;
		data->lastState = curState;
	}
	if((curTime - data->lastTime) > threshold){ 
		if(data->lastSteadyState == 1 && curState == 0){
			buttonState = 0;
		}
		else if(data->lastSteadyState == 0 && curState == 1){
			buttonState = 1;
		}
		data->lastSteadyState = curState;
	}
	return buttonState;
}