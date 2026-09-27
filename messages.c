/*
 * messages.c
 *
 *  Created on: 22 мар. 2026 г.
 *      Author: eugen
 */

#include <stddef.h>
#include "messages.h"
#include <stdarg.h>
#include <string.h>

typedef struct
{
	MessageProcessor processor;
    uint8_t *messages;
}ProcessorDesc;

static uint8_t message_processor_buffer[MESSAGE_PROCESSOR_BUFFER_SIZE];
static ProcessorDesc processors[PROCESSOR_COUNT];
static Message_t messages[MESSAGE_COUNT];
static uint8_t rmsgindex = 0, wmsgindex = 0;

static bool _processor_valid(ProcessorDesc *processor)
{
	return (processor != NULL) && (processor <= &(processors[PROCESSOR_COUNT - 1]));
}

static bool _processor_active(ProcessorDesc *processor)
{
	return _processor_valid(processor) && (processor->processor != NULL) &&
		   (processor->messages != NULL) && (*(processor->messages) != 0);
}

static ProcessorDesc *_processor_get_next(ProcessorDesc *processor)
{
	if (_processor_valid(processor))
	{
		processor++;
		if (_processor_valid(processor))
			return processor;
	}
	else
		return processors;
	return NULL;
}

static uint8_t _processor_count(void)
{
	ProcessorDesc *processor = _processor_get_next(NULL);
	uint8_t count = 0;

	while (_processor_active(processor))
	{
		count++;
		processor = _processor_get_next(processor);
	}
	return count;
}

static ProcessorDesc *_processor_free_ptr(void)
{
	ProcessorDesc *processor = _processor_get_next(NULL);

	while (processor)
	{
		if (!_processor_active(processor))
			break;
    processor = _processor_get_next(processor);
	}
	return processor;
}

bool Message_AddProcessor(MessageProcessor processor, uint8_t message_count, const uint8_t *message_list)
{
	if (message_count)
	{
		uint8_t *msg_block = message_processor_buffer;
		uint16_t index;
		while (*msg_block)
			msg_block += (*msg_block + 1);
		index = (uint32_t)msg_block - (uint32_t)message_processor_buffer;
		if ((MESSAGE_PROCESSOR_BUFFER_SIZE - index) > message_count)
		{
			ProcessorDesc *cproc = _processor_free_ptr();
			if (_processor_valid(cproc))
			{
				cproc->processor = processor;
				cproc->messages = msg_block;
				*msg_block++ = message_count;
				while (message_count--)
					*msg_block++ = *message_list++;
				*msg_block = 0;
				cproc = _processor_get_next(cproc);
				if (_processor_valid(cproc))
				{
					cproc->processor = NULL;
					cproc->messages = NULL;
				}
				return true;
			}
		}
	}
	return false;
}

static uint8_t _next_msg_index(uint8_t index)
{
	index++;
	if (index >= MESSAGE_COUNT)
		index = 0;
	return index;
}

void Message_Init(void)
{
	memset(message_processor_buffer, 0, sizeof(message_processor_buffer));
	memset(processors, 0, sizeof(processors));
	wmsgindex = 0;
	rmsgindex = 0;
}

bool Message_Add(uint8_t ID, uint8_t par1, uint16_t par2, uint32_t par3)
{
  uint8_t index = _next_msg_index(wmsgindex);
  if (index != rmsgindex)
  {
	  messages[wmsgindex].ID = ID;
	  messages[wmsgindex].upar8 = par1;
	  messages[wmsgindex].upar16 = par2;
	  messages[wmsgindex].upar32 = par3;
	  wmsgindex = index;
	  return true;
  }
  return false;
}

bool Message_AddPtr(uint8_t ID, uint8_t par1, uint16_t par2, void *par3)
{
	return Message_Add(ID, par1, par2, (uint32_t)par3);
}

static ProcessorDesc * current_processor = NULL;

static bool _processor_isactive(ProcessorDesc * processor, uint8_t msgID)
{
	if (_processor_active(processor))
	{
		uint8_t i;
		for (i = 1; i <= processor->messages[0]; i++)
			if (processor->messages[i] == msgID)
				return true;
	}
	return false;
}

void Message_Poll(void)
{
  if (wmsgindex != rmsgindex)
  {
	  current_processor = _processor_get_next(current_processor);
	  if (!_processor_active(current_processor))
	  {
		  current_processor = NULL;
		  rmsgindex = _next_msg_index(rmsgindex);
	  }
	  else
		  if (_processor_isactive(current_processor, messages[rmsgindex].ID))
			  current_processor->processor(&messages[rmsgindex]);
  }
}
