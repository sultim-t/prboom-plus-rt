// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//----------------------------------------------------------------------------
//
// ser_port.c
//
// code to interface with the comm port.
// I suppose this is slightly more 'system-specific' than ser_main.c
//
// 97% id sersetup sources, 3% Simon Howard
//
//---------------------------------------------------------------------------

#include <dos.h>
#include <pc.h>
#include <go32.h>
#include <dpmi.h>

#include "ser_main.h"

#include "../d_main.h"
#include "../d_net.h"
#include "../c_io.h"

void jump_start( void );

static void isr_8250 (void);

union REGS regs;
struct SREGS sregs;

que_t inque, outque;

int uart;                   // io address
enum {UART_8250, UART_16550} uart_type;
int irq;

int modem_status = -1;
int line_status = -1;

// sf: use go32 protected mode irq
_go32_dpmi_seginfo oldirqvect;
_go32_dpmi_seginfo newirqvect;

int irqintnum;

int comport = 2;


/*
==============
=
= GetUart
=
==============
*/

void GetUart (void)
{
  static int ISA_uarts[] = {0x3f8,0x2f8,0x3e8,0x2e8};
  static int ISA_IRQs[] = {4,3,4,3};
  //        static int MCA_uarts[] = {0x03f8,0x02f8,0x3220,0x3228};
  //        static int MCA_IRQs[] = {4,3,3,3};
  //        int             p;


  irq = ISA_IRQs[ comport-1 ];
  uart = ISA_uarts[ comport-1 ];

  /*  
  regs.h.ah = 0xc0;
  int86x( 0x15, &regs, &regs, &sregs );
  if ( regs.x.cflag )
    {
      return;
    }
  system_data = ( char far *) ( ( (long) sregs.es << 16 ) + regs.x.bx );
  if ( system_data[ 5 ] & 0x02 )
    {
      irq = MCA_IRQs[ comport-1 ];
      uart = MCA_uarts[ comport-1 ];
    }
  else
    {
      irq = ISA_IRQs[ comport-1 ];
      uart = ISA_uarts[ comport-1 ];
    }

  p = CheckParm ("-port");
  if (p)
    sscanf (_argv[p+1],"0x%x",&uart);
  p = CheckParm ("-irq");
  if (p)
    sscanf (_argv[p+1],"%i",&irq);
  */

  usermsg("Looking for UART at port 0x%x, irq %i",uart,irq);
}

/*
===============
=
= InitPort
=
===============
*/

void InitPort (void)
{
  int mcr;
  int temp;

  //
  // Reset the output queue
  //

  outque.head = outque.tail = 0;

  //
  // find the irq and io address of the port
  //
  GetUart ();

  //
  // init com port settings
  //
  regs.x.ax = 0xf3;               //f3= 9600 n 8 1
  regs.x.dx = comport - 1;
  int86 (0x14, &regs, &regs);
  
  //
  // check for a 16550
  //
  OUTPUT(uart + FIFO_CONTROL_REGISTER, FCR_FIFO_ENABLE + FCR_TRIGGER_04);
  temp = INPUT(uart + INTERRUPT_ID_REGISTER);
  if ((temp & 0xf8) == 0xc0)
    {
      uart_type = UART_16550;
      usermsg ("UART is a 16550\n\n");
    }
  else
    {
      uart_type = UART_8250;
      OUTPUT(uart + FIFO_CONTROL_REGISTER, 0);
      usermsg("UART is an 8250\n\n");
    }
  
  //
  // prepare for interrupts
  //

  OUTPUT(uart + INTERRUPT_ENABLE_REGISTER, 0);
  mcr = INPUT(uart + MODEM_CONTROL_REGISTER);
  mcr |= MCR_OUT2;
  mcr &= ~MCR_LOOPBACK;
  OUTPUT(uart + MODEM_CONTROL_REGISTER, mcr);
  
  INPUT(uart);  // Clear any pending interrupts
  INPUT(uart + INTERRUPT_ID_REGISTER);
  
  //
  // hook the irq vector
  //
  irqintnum = irq + 8;
  
  asm("cli"); // disable interrupts

  _go32_dpmi_get_protected_mode_interrupt_vector(irqintnum, &oldirqvect);
  newirqvect.pm_offset = (int)isr_8250;
  newirqvect.pm_selector = _go32_my_cs();
  _go32_dpmi_allocate_iret_wrapper(&newirqvect);
  _go32_dpmi_set_protected_mode_interrupt_vector(irqintnum, &newirqvect);

  asm("sti"); // enable interrupts
	
  OUTPUT(0x20 + 1, INPUT(0x20 + 1) & ~(1<<irq));

  asm("cli"); // disable again

  // enable RX and TX interrupts at the uart
  
  OUTPUT(uart + INTERRUPT_ENABLE_REGISTER,
	 IER_RX_DATA_READY + IER_TX_HOLDING_REGISTER_EMPTY);

  // enable interrupts through the interrupt controller
  
  OUTPUT(0x20, 0xc2);

  // set DTR
  OUTPUT(uart + MODEM_CONTROL_REGISTER
	  , INPUT(uart + MODEM_CONTROL_REGISTER) | MCR_DTR);
  
  asm("sti"); // re-enable
}


/*
=============
=
= ShutdownPort
=
=============
*/

void ShutdownPort ( void )
{
  OUTPUT(uart + INTERRUPT_ENABLE_REGISTER, 0);
  OUTPUT(uart + MODEM_CONTROL_REGISTER, 0);
  
  OUTPUT(0x20 + 1, INPUT(0x20 + 1) | (1<<irq));
  
  // restore old irq
  asm("cli");
  _go32_dpmi_set_protected_mode_interrupt_vector
    (irqintnum, &oldirqvect);
  _go32_dpmi_free_iret_wrapper(&newirqvect);
  asm("sti");

  //
  // init com port settings to defaults
  //
  regs.x.ax = 0xf3;               //f3= 9600 n 8 1
  regs.x.dx = comport - 1;
  int86 (0x14, &regs, &regs);

  usermsg("shutting down serial port");
}


int read_byte( void )
{
  int c;
  
  if (inque.tail >= inque.head)
    return -1;
  c = inque.data[inque.tail%QUESIZE];
  inque.tail++;
  return c;
}


void write_byte( int c )
{
  outque.data[outque.head%QUESIZE] = c;
  outque.head++;
}

//==========================================================================


/*
==============
=
= isr_8250
=
==============
*/

void isr_8250(void)
{
  int c;
  int count;

  while (1)
    {
      switch( INPUT( uart + INTERRUPT_ID_REGISTER ) & 7 )
	{
	  // not enabled
	case IIR_MODEM_STATUS_INTERRUPT :
	  modem_status = INPUT( uart + MODEM_STATUS_REGISTER );
	  break;
	  
	  // not enabled
	case IIR_LINE_STATUS_INTERRUPT :
	  line_status = INPUT( uart + LINE_STATUS_REGISTER );
	  break;
	  
	  //
	  // transmit
	  //
	case IIR_TX_HOLDING_REGISTER_INTERRUPT :
	  //I_ColorBlack (63,0,0);
	  if (outque.tail < outque.head)
	    {
	      if (uart_type == UART_16550)
		count = 16;
	      else
		count = 1;
	      do
		{
		  c = outque.data[outque.tail%QUESIZE];
		  outque.tail++;
		  OUTPUT( uart + TRANSMIT_HOLDING_REGISTER, c );
		} while (--count && outque.tail < outque.head);
	    }
	  break;
	  
	  //
	  // receive
	  //
	case IIR_RX_DATA_READY_INTERRUPT :
	  do
	    {
	      c = INPUT( uart + RECEIVE_BUFFER_REGISTER );
	      inque.data[inque.head%QUESIZE] = c;
	      inque.head++;
	    } while ( uart_type == UART_16550 && INPUT( uart + LINE_STATUS_REGISTER ) & LSR_DATA_READY );
	  
	  break;
	  
	  //
	  // done
	  //
	default:
	  OUTPUT( 0x20, 0x20 );
	  return;
	}
    }
}


/*
===============
=
= jump_start
=
= Start up the transmition interrupts by sending the first char
===============
*/

void jump_start( void )
{
  int c;

  if (outque.tail < outque.head)
    {
      c = outque.data[outque.tail % QUESIZE];
      outque.tail++;
      OUTPUT(uart, c);
    }
}

// EOF