//################################################################################################
//
// Program     : Measurement in Zephyr RTOS
// Assignment  : 3
// Source file : main.c
// Authors     : Sarvesh Patil & Vishwakumar Doshi
// Date        : 30 March 2018
//
//################################################################################################

#include <zephyr.h>
#include <kernel.h>
#include <pwm.h>
#include <pinmux.h>
#include <device.h>
#include <gpio.h>
#include <board.h>
#include <misc/util.h>
#include <misc/printk.h>
#include <asm_inline_gcc.h>
#include <stdio.h>
#include <stdlib.h>
#include <shell/shell.h>


#define EDGE (GPIO_INT_EDGE|GPIO_INT_ACTIVE_HIGH)
#define PWM CONFIG_PWM_PCA9685_0_DEV_NAME
#define MUX CONFIG_PINMUX_NAME

#define MY_SHELL_MODULE "shell_mod"

// Stack size used by thread
#define MY_STACK_SIZE 1024

// Scheduling priorities
#define PRIORITY_TN1 7                                        //Priorities of threads
#define PRIORITY_TN2 14
#define PRIORITY_TB1 4 
#define PRIORITY_TB2 6

//==============================================
// Global functions & variables declaration area
//===============================================
struct device *dev1, *dev3, *dev4, *pwm_dev;                   // device struct variables
static struct gpio_callback cb;

// Message queue variables
struct data_item_type 
{
    char field[12];
};

long latency_val[500][3];                                       //matrix to hold latency ticks values
struct data_item_type msgq_data;
struct k_msgq msgq_struct;

char msgqueue_buf[sizeof(msgq_data)];
char message[12] = "ENQUEUE";


// Thread declarations
extern void thread_Norm1();
extern void thread_Norm2();
extern void thread_Back1();
extern void thread_Back2();

K_THREAD_STACK_DEFINE(my_stack_area_TN1, MY_STACK_SIZE);          //static definiition of stack areas
K_THREAD_STACK_DEFINE(my_stack_area_TN2, MY_STACK_SIZE);
K_THREAD_STACK_DEFINE(my_stack_area_TB1, MY_STACK_SIZE);
K_THREAD_STACK_DEFINE(my_stack_area_TB2, MY_STACK_SIZE);

struct k_thread my_thread_data_TN1;                              //kthread structures
struct k_thread my_thread_data_TN2;
struct k_thread my_thread_data_TB1;
struct k_thread my_thread_data_TB2;

k_tid_t my_tid_TN1;                                              //thread ids
k_tid_t my_tid_TN2;
k_tid_t my_tid_TB1;
k_tid_t my_tid_TB2;


// Synchronization declarations
struct k_mutex mut_norm;                                         // Mutex lock for context switching threads
struct k_sem sem_cont_main;									     // semaphore to block main while function threads are running to avoid main overhead

u8_t norm_flag=0,norm_exit=0;                                    // Flags for context switch thread coordination
int cnt=0;                                                       // count variable to keep count till 500
u8_t column;                                                     // specifies which column to write to in matrix

long long ct_1, ct_2;                                            // time stamp variables

// Shell Functions
//==================


static int shell_cmd_params(int argc, char *argv[])              // shell module command function
{
    int count;                                                   // getting how many values to print
    int num=atoi(argv[1]);
    printk("\n\nCont-swch  Int-no-bg-task  Int-bg-task\n",num);

    for (count = 0; count < num; count++) 
    {
      	printk("%ld\t\t%ld\t\t%ld\n", ((latency_val[count][0]*10)/4),((latency_val[count][1]*10)/4),((latency_val[count][2]*10)/4));
    }
    return 0;
}

static struct shell_cmd commands[] = {          
    { "print", shell_cmd_params, "print argc" },
    { NULL, NULL, NULL }
};                                                                              //structure to hold command

static void myshell_init(void)                                                  // init function for shell
{
    printk("\n\n###################################################################\n");
    printk("###################################################################\n");
    printk("Press Enter to initialise SHELL.\n\nEnter command: print n \n\ncommand to print first n latency values from 500 values.\n");
    printk("###################################################################\n");
    printk("###################################################################\n");
    
    SHELL_REGISTER(MY_SHELL_MODULE, commands);
}

//===================
// Callback Function
//===================
void cb_func(struct device *dev2, struct gpio_callback *cb1, u32_t pins)          // callback function for gpio interrupt
{
	ct_2=_tsc_read();
	latency_val[cnt][column]=(ct_2 - ct_1);
	printk("Interrupt Latency ticks is %ld\n",latency_val[cnt][column]);
	cnt=cnt+1;
}


//=======================
// Thread functions Area
//=======================

// Normal context switching
void thread_Norm1()                                                               // high priority context switching thread
{
    for(int i=0;i<500;i++)
    {
	    k_sleep(5); 
	    norm_flag=1;
	    k_mutex_lock(&mut_norm,K_FOREVER);
	    ct_2 = _tsc_read();
	    latency_val[i][0]=(ct_2 - ct_1);
	    printk("Context Switch Latency ticks is : %ld\n",latency_val[i][0]);
	    norm_flag=0;
	    k_mutex_unlock(&mut_norm);
	}
	norm_flag=0;
	norm_exit=1;

}

void thread_Norm2()																// Low priority context switching thread
{
	while(norm_exit!=1)
    {
	    k_mutex_lock(&mut_norm,K_FOREVER);
	    while(norm_flag!=1)
	    {
	    	ct_1=_tsc_read();
	    }
	    ct_1 = _tsc_read();
	    k_mutex_unlock(&mut_norm);
	    ct_1=_tsc_read();
	    while(norm_flag!=0)
	    {
	    	ct_1=_tsc_read();
	    }
	}
	k_sem_give(&sem_cont_main);
}


// With background tasks
void thread_Back1()                                                           //High priority message sender background task thread
{
	printk("\n\nInside Background computating sender thread\n\n");
	k_sleep(2);
	ct_1=_tsc_read();
	gpio_pin_enable_callback(dev3,3);


    while(1)
    {
        ct_1=_tsc_read();

        k_msgq_put(&msgq_struct, message, K_FOREVER); 

        if(cnt==500)
            break; 
    }
    gpio_pin_disable_callback(dev3,3);
}

void thread_Back2()															//Low priority message receiver background task thread
{
       
    printk("\n\nInside Background computating receiver thread\n\n");
    k_sleep(2);
    ct_1=_tsc_read();
    

    while(1)
    {
        ct_1 =_tsc_read();
        k_msgq_get(&msgq_struct, message, K_FOREVER);
        ct_1 =_tsc_read();

        if(cnt==500)
            break; 
    }
    
    gpio_pin_disable_callback(dev3,3);
    k_sem_give(&sem_cont_main);
}

// #######################################
//
// Main
//
//########################################
void main()
{
   int ret;

   // Message queue init
   k_msgq_init(&msgq_struct, msgqueue_buf,sizeof(msgq_data),1);                   //Initialises msg queue
    
   printk("\n\n##############################################\n");
   printk("Measuring Context Switch Latency\n");
   printk("##############################################\n\n\n");


   // Starting Normal context switching computing
   //=============================================

   k_mutex_init(&mut_norm);
   k_sem_init(&sem_cont_main, 0, 1);
   
   // Creating threads
   my_tid_TN2 = k_thread_create(&my_thread_data_TN2, my_stack_area_TN2, K_THREAD_STACK_SIZEOF(my_stack_area_TN2), 
                                thread_Norm2, NULL, NULL, NULL, PRIORITY_TN2, 0, K_NO_WAIT);

   my_tid_TN1 = k_thread_create(&my_thread_data_TN1, my_stack_area_TN1, K_THREAD_STACK_SIZEOF(my_stack_area_TN1), 
                                thread_Norm1, NULL, NULL, NULL, PRIORITY_TN1, 0, 3);
   
   k_sem_take(&sem_cont_main,K_FOREVER);      // main waits after spawning threads
   k_sleep(10);
   

   // Starting Interrupt handler computing
   //======================================

   printk("\n\n##########################################################\n");
   printk("Measuring Interrupt Latency Without Background Computing!\n");
   printk("###########################################################\n\n\n");
   
	dev1 = device_get_binding(MUX);                              // Muxing pins for gpio input for rising edge interrupt callback funtion and pwm pin
	if(dev1 == NULL)
		printk("Pinmux device not found");

	dev3= device_get_binding("GPIO_0");
	if(dev3 == NULL)
		printk("GPIO_CW device not found");
	gpio_pin_configure(dev3,3,(GPIO_DIR_IN|GPIO_INT| EDGE));
	

	dev4 = device_get_binding("EXP1");
	if(dev4 == NULL)
		printk("EXP1 device not found");
	gpio_pin_configure(dev4,0,GPIO_DIR_OUT);
	gpio_pin_configure(dev4,1,GPIO_DIR_OUT);
	ret = gpio_pin_write(dev4,0,1);
	if(ret!=0)
		printk("Error in dev 4 0 wr\n");
	ret = gpio_pin_write(dev4,1,0);
	if(ret!=0)
		printk("Error in dev 4 1 wr\n");
	

	gpio_init_callback(&cb,cb_func, BIT(3));
	ret=gpio_add_callback(dev3,&cb);
	if(ret!=0)
		printk("Error in add callback\n");
	
	ret = pinmux_pin_set(dev1,5, PINMUX_FUNC_C);
	if(ret!=0)
		printk("Error setting pinmux 5");


	pwm_dev = device_get_binding(PWM);
	if(pwm_dev == NULL)
		printk("PWM0 device not found");

	ret = pwm_pin_set_cycles(pwm_dev,3,4095,3096);
	if(ret!=0)
		printk("Error setting pwm cycles");

	column=1;
	ct_1= _tsc_read();
	ret=gpio_pin_enable_callback(dev3,3);
	if(ret!=0)
		printk("Error in enable callback\n");

	while(cnt<500)															// Takes 500 latency data points
	{
		ct_1=_tsc_read();
	}

	gpio_pin_disable_callback(dev3,3);


	// Starting context switching with background computing
	//=======================================================

   printk("\n\n##########################################################\n");
   printk("Measuring Interrupt Latency With Background Computing!\n");
   printk("###########################################################\n\n\n");

	k_sleep(10);
	cnt=0;
	column=2;                                    // Now stores in column 2


    ct_1=_tsc_read();
    my_tid_TB2 = k_thread_create(&my_thread_data_TB2, my_stack_area_TB2, K_THREAD_STACK_SIZEOF(my_stack_area_TB2), 
                                thread_Back2, NULL, NULL, NULL, PRIORITY_TB2, 0, K_NO_WAIT);

    my_tid_TB1 = k_thread_create(&my_thread_data_TB1, my_stack_area_TB1, K_THREAD_STACK_SIZEOF(my_stack_area_TB1), 
                                thread_Back1, NULL, NULL, NULL, PRIORITY_TB1, 0, 3);

    ct_1=_tsc_read();
   k_sem_take(&sem_cont_main,K_FOREVER);       // Main  waits again after spawning background threads while pwm is on to produce interrupts
   k_sleep(10);
    
   myshell_init();                             // After storing all values in matrix, initialises shell to let user use print command 
}

//###############
// End of main.c
//###############