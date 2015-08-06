#ifndef STM32_TIMER_HAL_TIMER_BASE_HPP
#define STM32_TIMER_HAL_TIMER_BASE_HPP

#include "peripheral.hpp"

namespace HAL {
    namespace Timer {
        typedef TIM_TypeDef raw_timer_t;

        /**
         * TimerBase (type)
         *
         * This represents a raw timer, and only supports low-level operations, such as enabling/disabling.
         *
         */
        template<typename P>
        class TimerBase {
            //***************************
            //* Subtypes                *
            //***************************
        public:
            typedef P peripheral;

            //***************************
            //* Members                 *
            //***************************
        protected:
            // Real members
            bool enabled = false;

        public:
            static constexpr raw_timer_t* const periph_base = (raw_timer_t*) P::periph_base;
            static constexpr uint32_t bus_freq() {
                return P::bus::bus_freq();
            }

            //***************************
            //* Methods                 *
            //***************************
        public:
            TimerBase() {
                P::enable();
            }

            ~TimerBase() {
                disable();
                P::disable();
            }

            bool is_enabled() const {
                return enabled;
            }

            void enable() {
                if (!is_enabled()) {
                    periph_base->CR1 |= TIM_CR1_CEN;
                }
            }

            void disable() {
                if (is_enabled()) {
                    periph_base->CR1 &= ~TIM_CR1_CEN;
                }
            }
        };

        
        /**
         * BasicTimer (type)
         *
         * This represents a basic timer peripheral and supports only basic functions.
         * It is constituted of a 16 bit auto-reload upcounting register with user-defineable reload value
         * and it has a 16 bit programmable prescaler to divide the counter's input clock frequency.
         * Counter's original (not divided by prescaler) clock frequency can be get calling the bus_freq() method
         * 
         * refer to programming manual for further informations
         * 
         */
        template<typename P>
        class BasicTimer : public TimerBase<P> {
            //***************************
            //* Members                 *
            //***************************
        private:
//             uint32_t counter_freq;
//             uint16_t reload_val;

            //***************************
            //* Methods                 *
            //***************************
        public:
            using TimerBase<P>::periph_base;
            using TimerBase<P>::bus_freq;
            
            /**
             * The counter is incremented by one every 1/(counter_freq) seconds
             * and counts from 0 to reload_val, then restarts from 0
             * 
             * @param counter_freq: counter's tick frequency
             * @param reload_val: counter's auto reload value
             * 
             */

            BasicTimer(uint32_t counter_freq, uint16_t reload_val = 65535) /* :
                    counter_freq(counter_freq), reload_val(reload_val) */
            {
                // Set counter frequency through prescaler
                periph_base->PSC = (bus_freq() / counter_freq) - 1;
                
                // Set counter's auto reload value
                periph_base->ARR = reload_val;
                
                // clear the counter
                periph_base->CNT = 0;
                
                // enable auto reload
                periph_base->CR1 |= TIM_CR1_ARPE;
            }
            
            /**
             * 
             * When called enables the counter (it starts counting)
             * 
             */

            void start()
            {
                TimerBase<P>::enable();
            }
            
            /**
             * 
             * When called disables the counter (it stops counting)
             * 
             */

            void stop()
            {
                TimerBase<P>::disable();
            }

            /**
             * 
             * When called set timer's counter value to zero
             * 
             */
            
            void clear()
            {    
                periph_base->CNT = 0;
            }

            /**
             * 
             * @return: current counter's value
             * 
             * NOTE: it returns a 32 bit value for compatibility with
             * 32 bit timers present in some devices, but usually timers are 16 bit counters
             * 
             * Check device's datasheet for more informations
             * 
             */
            
            uint32_t getValue()
            {
                return periph_base->CNT;
            }

            
            /**
             * Chech if counter has reached (is equal to or greater than) a
             * defined value.
             * 
             * @return: true if counter's value equal to or greater than
             * parameter passed, false otherwise
             * @param value: the value the counter's value is compared to
             * 
             */
            
            bool has_reached(uint32_t value)
            {
                return (periph_base->CNT >= value);
            }
            
            /**
             * Sets the prescaler value.
             * Counter's frequency is CK_CNT = F_CK / (prescaler_value - 1)
             * with F_CK frequency before the prescaler, readable through bus_freq() method.
             * 
             * @param value: prescaler value to be set
             * 
             */
            
            void setPrescaler(uint16_t value)
            {
                periph_base->PSC = value;
            }
            
            /**
             * Gets the prescaler value.
             * @return actual prescaler value
             * 
             */
            
            uint16_t getPrescaler()
            {
                return (periph_base->PSC);
            }            
            
            /**
             * Sets auto reload register value. 
             * @param value: auto reload register value to be setPrescaler
             * 
             */
            
            void setAutoReload(uint32_t value)
            {
                periph_base->ARR = value;
            }
            
            /**
             * Gets auto reload register value.
             * @return actual auto-reload register value
             * 
             */
            
            uint32_t getAutoReload()
            {
                return (periph_base->ARR );
            }            
            
            /**
             * Check if counter has reached auto reload value and, consequently, has rolled back to zero.
             * In datasheet this is called update event.
             * Calling this function clears the bit
             * 
             * @return: true if reload event has occurred, false otherwise
             * 
             */
            
            bool checkReloadEvent()
            {
                if(periph_base->TIM_SR & TIM_SR_UIF)
                {
                    periph_base->TIM_SR &= ~TIM_SR_UIF;
                    return true;
                }
                
                return false;
            }
        };
        
        /**
         * PwmGenerator (type)
         *
         * A timer peripheral is used to generate a pwm signal and can support up to four channels
         * Please not that NOT all the timer peripherals can support up to four channels, some can
         * handle a maximum of two channels.
         * 
         * In order to make this class as general as possible actually only edge-aligned, pwm mode 1
         * signal generation is covered
         * 
         * Please refer to MCU's datasheet and programming manual
         * for further informations.
         * 
         */
        template<typename P>
        class PwmGenerator : public TimerBase<P> {
            //***************************
            //* Members                 *
            //***************************
        private:
//             uint32_t counter_freq;
            uint32_t period;

            //*************************** 
            //* Methods                 *
            //***************************
        public:
            using TimerBase<P>::periph_base;
            using TimerBase<P>::bus_freq;
            
            /**
             * @param counter_freq: the timer's counter counting frequency. It determines the tick width.
             * @param period: the generated pwm signal's period length expressed in counter ticks
             * @param isAdvanced: set it to true if you are using advanced control timers (TIM1 & TIM8)
             */

            PwmGenerator(uint32_t counter_freq, uint32_t period, bool isAdvanced = false) : /* counter_freq(counter_freq),*/ period(period)
            {
                // Set counter frequency through prescaler
                periph_base->PSC = (bus_freq() / counter_freq) - 1;
                
                // Set pwm period through reload register value
                periph_base->ARR = period;
                
                //clear counter register
                periph_base->CNT = 0;
                
                // Dummy update event in order to load registers
                periph_base->EGR = TIM_EGR_UG;
                
                /* this is a workaround made because advanced control timers must
                 * have MOE bit set in order to have outputs working properly.
                 * If MOE bit is cleared no output signal is generated at all */
                
                if(isAdvanced)
                    periph_base->BDTR |= TIM_BDTR_MOE;

                // Auto reload enabled
                periph_base->CR1 |= TIM_CR1_ARPE;
            }
            
            /**
             * Starts the timer. Calling this function makes the timer generating
             * pwm signal(s) on its output(s)
             */
            void start()
            {
                if(TimerBase<P>::is_enabled())
                    return;
                
                TimerBase<P>::enable();
            }
            
            /**
             * Stops the timer. Calling this function makes the timer stop
             * generating pwm signal(s) on its output(s)
             */
            void stop()
            {
                if(!TimerBase<P>::is_enabled())
                    return;
                
                TimerBase<P>::disable();
            }           
            
            /**
             * Enables a channel. Calling this function will "connect" the channel to the
             * peripheral, making the corresponding output pin generating the pwm signal.
             * This function must be called with timer stopped.
             * 
             * NOTE: pin initialization to alternate mode and, eventually, alternate mode
             * mapping isn't done here, so it MUST be done somewhere before calling this function.
             * 
             * @param channel: the channel number, between 1 and 4. Please note that not all
             * the timers have four channels!!
             * 
             * TODO: make this function thread safe
             */
            
            void enable(uint8_t channel)
            {
                if(TimerBase<P>::is_enabled())
                    return;
                
                switch(channel)
                {
                    case 1:
                        periph_base->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
                        periph_base->CCER |= TIM_CCER_CC1E;
                        break;
                    
                    case 2:
                        periph_base->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;
                        periph_base->CCER |= TIM_CCER_CC2E;
                        break;  
                    
                    case 3:
                        periph_base->CCMR1 |= TIM_CCMR1_OC3M_2 | TIM_CCMR1_OC3M_1 | TIM_CCMR1_OC3PE;
                        periph_base->CCER |= TIM_CCER_CC3E;
                        break;
                        
                   case 4:
                        periph_base->CCMR1 |= TIM_CCMR1_OC4M_2 | TIM_CCMR1_OC4M_1 | TIM_CCMR1_OC4PE;
                        periph_base->CCER |= TIM_CCER_CC4E;
                        break;
                }
            }
            
            /**
             * Disables a channel. Calling this function will "disconnect" the channel from the
             * peripheral, providing the corresponding output pin from generating the pwm signal.
             * This function must be called with timer stopped.
             * 
             * @param channel: the channel number, between 1 and 4. Please note that not all
             * the timers have four channels!!
             * 
             * TODO: make this function thread safe
             */
            
            void disable(uint8_t channel)
            {
                if(TimerBase<P>::is_enabled())
                    return;
                
                switch(channel)
                {
                    case 1:
                        periph_base->CCER &= ~TIM_CCER_CC1E;
                        break;
                    
                    case 2:
                        periph_base->CCER &= ~TIM_CCER_CC2E;
                        break;  
                    
                    case 3:
                        periph_base->CCER &= ~TIM_CCER_CC3E;
                        break;
                        
                   case 4:
                        periph_base->CCER &= ~TIM_CCER_CC4E;
                        break;
                }
            }
            
            /**
             * Sets channel's pwm signal logical High value duration expressed in counter ticks.
             * If value is equal to zero the output will be permanently low, if value is equal to
             * pwm signal's period the output will be permanently high.
             * If value is greater than pwm signal's period the value passed will be ignored
             * 
             * @param channel: the channel number, between 1 and 4. Please note that not all
             * the timers have four channels!!
             * @param value: the channel's High time period, expressed in counter ticks
             * 
             * TODO: make this function thread safe
             */
            void setOnPeriod(uint16_t channel, uint16_t value)
            {
                if(value > period)
                    return;
                
                switch(channel)
                {
                    case 1:
                        periph_base->CCR1 = value;
                        break;
                    
                    case 2:
                        periph_base->CCR2 = value;
                        break;  
                    
                    case 3:
                        periph_base->CCR3 = value;
                        break;
                        
                   case 4:
                        periph_base->CCR4 = value;
                        break;
                }  
            }

        };

        /**
         * PwmMeasure (type)
         *
         * A timer peripheral is used to capture (aka measure)
         * the period length and the Ton length of a PWM signal
         * Please refer to MCU's datasheet and programming manual
         * for further informations.
         * The period value expressed in counter ticks is measured in CCR1,
         * the Ton value in CCR2 
         */
        template<typename P>
        class PwmMeasure : public TimerBase<P> {
            //***************************
            //* Members                 *
            //***************************
        private:
            uint32_t counter_freq;

            //***************************
            //* Methods                 *
            //***************************
        public:
            using TimerBase<P>::periph_base;
            using TimerBase<P>::bus_freq;

            /**
             * @param counter_freq: the timer's counting frequency. It determines the tick width.
             */
            
            PwmMeasure(uint32_t counter_freq) : counter_freq(counter_freq)
            {
                // Set count frequency
                periph_base->PSC = (bus_freq() / counter_freq) - 1;

                // IC1 mapped on TI1
                periph_base->CCMR1 |= TIM_CCMR1_CC1S_0;

                // IC2 mapped on TI1
                periph_base->CCMR1 |= TIM_CCMR1_CC2S_1;

                // Trigger is T1FP1
                periph_base->SMCR |= TIM_SMCR_TS_2 | TIM_SMCR_TS_0;

                // Timer in reset mode, input rising edge clears it
                periph_base->SMCR |= TIM_SMCR_SMS_2;

                // Channel 2 enabled, falling edge
                periph_base->CCER |= TIM_CCER_CC2P | TIM_CCER_CC2E;

                // Capture enabled on channel 1, rising edge
                periph_base->CCER |= TIM_CCER_CC1E;
            }

            /**
             * width_event (method)
             *
             * This checks whether the interrupt flag associated to PWM pulse width measure is set.
             * If so, it clears the flag and returns true, otherwise returns false.
             *
             * @return true if the the flag is set, false otherwise
             */
            bool width_event() {
                if (periph_base->SR & TIM_SR_CC2IF) {
                    periph_base->SR &= ~TIM_SR_CC2IF;
                    return true;
                } else {
                    return false;
                }
            }

            /**
             * period_event (method)
             *
             * This checks whether the interrupt flag associated to PWM period measure is set.
             * If so, it clears the flag and returns true, otherwise returns false.
             *
             * @return true if the the flag is set, false otherwise
             */
            bool period_event() {
                if (periph_base->SR & TIM_SR_CC1IF) {
                    periph_base->SR &= ~TIM_SR_CC1IF;
                    return true;
                } else {
                    return false;
                }
            }

            /**
             * get_pulse_width (method)
             *
             * This inspects the timer register and returns the latest pulse width measure.
             *
             * @return the latest pulse width measure, expressed in counter ticks
             */
            uint32_t get_pulse_width() {
                return periph_base->CCR2;
            }

            /**
             * get_period (method)
             *
             * This inspects the timer register and returns the latest period measure.
             *
             * @return the latest period measure, expressed in counter ticks
             */
            uint32_t get_period() {
                return periph_base->CCR1;
            }
        };

    }
}

#endif //STM32_TIMER_HAL_TIMER_BASE_HPP
