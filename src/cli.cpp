/*
 * cli.cpp
 *
 *  Created on: Jan 29, 2015
 *      Author: julian
 */

#include <stdio.h>

#include "cli.h"

Cli::Cli() :
     esc(false)
   , buf_idx(0)
   , arg_idx(0)
   , p_cmd_tab(0)
   , cmd_tab_siz(0)
   , p_last_cmd(0)
{
    /* Nothing more to do */
}

void Cli::init(cli_cmd_t *p_table, uint8_t size)
{
    uint8_t len = 0;

    p_cmd_tab=p_table;
    cmd_tab_siz=size;
    reset();
}

void Cli::set_cmd_table(cli_cmd_t *p_table, uint8_t size)
{
    p_cmd_tab = p_table;
    cmd_tab_siz = size;
}

int8_t Cli::proc_byte(char data)
{
    int8_t ret=0;

    if (!esc && data == cmd_esc)
    {
       esc=true;
    }
    else if (esc || data != cmd_term)
    {
       if (data==del)
          buf_idx = buf_idx>0 ? buf_idx-1 : 0;
       else
          buffer[buf_idx++] = data;
       esc=false;
    }
    else if (p_cmd_tab)
    {
       if (buf_idx)
          buffer[buf_idx] = '\0';
       ret=check_cmd_table();
    }

    if (buf_idx == max_cmd_len-1)
       reset();

    return ret;
}

int8_t Cli::check_cmd_table(void)
{
    uint8_t i;
    int8_t ret=0;

    if (buf_idx == 0 && p_last_cmd != 0)
    {
       /* The argument index has to be set to 0 as it shall be possible
        * to check the arguments again.
        */
       arg_idx=0;
       ret=p_last_cmd((void*)&buffer);
       goto out;
    }
    else if (buf_idx == 0)
       goto out;

    for(i=0; i<cmd_tab_siz; i++)
    {
        if (check_cmd(&p_cmd_tab[i]))
        {
            p_last_cmd=p_cmd_tab[i].p_cmd_func;
            ret=p_last_cmd((void*)&buffer);
            goto out;
        }
    }

    printf("Unknown command\n");
    p_last_cmd=0;
    ret=-2;

    out:

    if(ret == -1)
    {
        printf("Command error\n");
        p_last_cmd = 0;
    }

    reset();
    return ret;
}

bool Cli::check_cmd(cli_cmd_t *p_cmd)
{
    uint8_t i = 0;

    while (p_cmd->cmd_text[i])
    {
        if (p_cmd->cmd_text[i] != buffer[i])
            return false;
        i++;
    }

    if (buffer[i] != '\0' && buffer[i] != arg_sep)
       return false;
    else
       return true;
}

char* Cli::get_parg(void)
{
   while (buffer[arg_idx] != 0)
   {
      if (buffer[arg_idx++] == cmd_esc)
         esc = true;
      else if (esc)
         continue;
      else if (buffer[arg_idx] == arg_sep)
      {
         while (buffer[arg_idx] == arg_sep)
            arg_idx++;

         if (buffer[arg_idx] == cmd_term || buffer[arg_idx] == '\0')
            break;

         return &(buffer[arg_idx]);
      }
   }

   return 0;
}

bool Cli::arg2int16(int16_t *p_val)
{
   bool negative=false;
   bool hex=false;
   uint8_t pos=arg_idx;
   int16_t temp=0;

   if (arg_idx == 0)
      return false;

   if (buffer[pos] == '+')
      pos++;
   else if (buffer[pos] == '-')
   {
      negative=true;
      pos++;
   }

   *p_val=0;

   while (  buffer[pos] != arg_sep
         && buffer[pos] != cmd_term
         && buffer[pos] != '\0')
   {
      if (*p_val == 0 && buffer[pos] == 'x')
      {
         if (negative)
            return false;
         else
            hex=true;
         pos++;
      }

      if (!hex)
      {
         if (!(buffer[pos] >= '0' && buffer[pos] <= '9'))
            return false;

         if (!negative)
         {
            temp=(*p_val*10)+(buffer[pos]-'0');
            if (temp < *p_val)
               return false;
         }
         else
         {
            temp=(*p_val*10)-(buffer[pos]-'0');
            if (temp > *p_val)
               return false;
         }

         *p_val=temp;
      }
      else
      {
         if (temp > (int16_t)sizeof(int16_t)*2)
            return false;
         temp++;

         *p_val=*p_val << 4;

         if (buffer[pos] >= '0' && buffer[pos] <= '9')
            *p_val+=buffer[pos]-'0';
         else if (buffer[pos] >= 'a' && buffer[pos] <= 'f')
            *p_val+=buffer[pos]-'a'+10;
         else if (buffer[pos] >= 'A' && buffer[pos] <= 'F')
            *p_val+=buffer[pos]-'A'+10;
         else
            return false;
      }

      pos++;
   }

   return true;
}

bool Cli::arg2uint16(uint16_t *p_val)
{
   bool hex=false;
   uint8_t pos=arg_idx;
   uint16_t temp=0;

   if (arg_idx == 0)
      return false;

   *p_val=0;

   while (  buffer[pos] != arg_sep
         && buffer[pos] != cmd_term
         && buffer[pos] != '\0')
   {
      if (*p_val == 0 && buffer[pos] == 'x')
      {
         hex=true;
         pos++;
      }

      if (!hex)
      {
         if (!(buffer[pos] >= '0' && buffer[pos] <= '9'))
            return false;

         temp=(*p_val*10)+(buffer[pos]-'0');
         if (temp < *p_val)
            return false;

         *p_val=temp;
      }
      else
      {
         if (temp > (int16_t)sizeof(int16_t)*2)
            return false;
         temp++;

         *p_val=*p_val << 4;

         if (buffer[pos] >= '0' && buffer[pos] <= '9')
            *p_val+=buffer[pos]-'0';
         else if (buffer[pos] >= 'a' && buffer[pos] <= 'f')
            *p_val+=buffer[pos]-'a'+10;
         else if (buffer[pos] >= 'A' && buffer[pos] <= 'F')
            *p_val+=buffer[pos]-'A'+10;
         else
            return false;
      }

      pos++;
   }

   return true;
}


bool Cli::wait_anykey(void)
{
   printf("Press any key to continue.\n");

//   while(!Serial.available());
//   Serial.read();

   return true;
}

bool Cli::wait_userconfirm(void)
{
   bool ret = false;
   printf("Press 'y' to confirm... ");

//   while (!Serial.available());
//   if (Serial.read() == 'y')
//      ret=true;

   printf("\n");
   return ret;
}

void Cli::reset(void)
{
    buf_idx=0;
    arg_idx=0;
    esc=false;
    printf("#>");
}
