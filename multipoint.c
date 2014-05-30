#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <math.h>
#include <unistd.h>


#define PI 3.14159265359


double coord[10][2];
int appui[10];

void fill_audio(void *udata, Uint8 *stream, int len)
{
  (void) udata;

  static double phase[10];
  static double freq[10] = {220., 220., 220., 220., 220., 220., 220., 220., 220., 220.};
  static double freqreelle[10] = {220., 220., 220., 220., 220., 220., 220., 220., 220., 220.};
  static double ampl[10];
  static double amplreelle[10];
  static double coeff[10];
  static double coeffreel[10];
  static double max;
  int i,j;
  for (j = 0; j < 10; j++)
  {
    freq[j] = pow(4.,coord[j][0]) * 220.;
    if (appui[j])
    {
      ampl[j] = pow(10., coord[j][1]-1.);
      coeff[j] = pow(4.,-coord[j][0])*pow(2.,-coord[j][1]);
//      ampl[j] = pow(10., coord[j][1]-1.)/pow(2.,coord[j][0]);
//      ampl[j] = 1.;
//      coeff[j] = 1.;
//      coeff[j] = coord[j][1];
    }
    else
    {
      ampl[j] = 0.;
    }
  }

  for (i = 0; i < len/2; i++)
  {
    double out = 0.;
    for (j = 0; j < 10; j++)
    {
      if (amplreelle[j] < ampl[j] - 1./400.)
      {
        amplreelle[j] += 1./400.;
      }
      else if (amplreelle[j] > ampl[j] + 1./4000.)
      {
        amplreelle[j] -= 1./4000.;
      }
      else
      {
        amplreelle[j] = ampl[j];
      }

      if (freqreelle[j] < freq[j] - 1.)
      {
        freqreelle[j] += 1.;
      }
      else if (freqreelle[j] > freq[j] + 1.)
      {
        freqreelle[j] -= 1.;
      }
      else
      {
        freqreelle[j] = freq[j];
      }
      
      if (coeffreel[j] < coeff[j] - 1./400.)
      {
        coeffreel[j] += 1./400.;
      }
      else if (coeffreel[j] > coeff[j] + 1./400.)
      {
        coeffreel[j] -= 1./400.;
      }
      else
      {
        coeffreel[j] = coeff[j];
      }
#if 0
      if (phase[j] < PI)
      {
        out += amplreelle[j] * (phase[j] - PI/2.)/PI*2.;
      }
      else
      {
        out -= amplreelle[j] * (phase[j] - 3.*PI/2.)/PI*2.;
      }
#endif
#if 1
      {

//        if (c < 0.1) c = 0.1;
#if 1
        double outsin = -sin(phase[j])*amplreelle[j]*2.*PI;
        outsin = sin(outsin)*coeffreel[j];
#endif
#if 0
        double outsin = -sin(phase[j])*amplreelle[j]*2.*PI*2.;
        outsin = atan(outsin)*coeffreel[j]/2.;
#endif
#if 0
        double outsin = -sin(phase[j])*amplreelle[j]*2.*PI*2.;
        if (abs(outsin) < 1e-6)
          outsin = 0.;
        else
          outsin = outsin * sin(outsin)*coeffreel[j]/10.;
#endif
//        outsin = outsin*(outsin-2.);
        out += outsin;
      }
#endif
#if 0
      out += amplreelle[j] * ((phase[j] > PI/2. && phase[j] < 3.*PI/2.) ? 1. : -1.) / 3.;
#endif
      phase[j] += freqreelle[j] / 44100. * 2. * PI;
      if (phase[j] > 2.*PI)
        phase[j] -= 2.*PI;
    }
    out *= 10000.;
    if (out > max)
       max = out;
    if (out > 32000.)
      out = 32000.;
    if (out < -32000.)
      out = -32000.;
    //printf("%lf\n", max);
    ((Sint16 *) stream)[i] = out;
  }
}

int init_sdl()
{
  if (SDL_Init(SDL_INIT_AUDIO) == -1)
  {
    printf("Erreur lors de l'initialisation de SDL!\n");
    return 1;
  }

  SDL_AudioSpec wanted;

  /* Set the audio format */
  wanted.freq = 44100;
  wanted.format = AUDIO_S16SYS;
  wanted.channels = 1;    /* 1 = mono, 2 = stereo */
  wanted.samples = 256;  /* Good low-latency value for callback */
  wanted.callback = fill_audio;
  wanted.userdata = NULL;

  /* Open the audio device, forcing the desired format */
  if ( SDL_OpenAudio(&wanted, NULL) < 0 )
  {
      fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
      return -1;
  }
  SDL_PauseAudio(0);
  return 0;
}

int main(int argc, char **argv)
{
  int fd;
  int rd;

  if (argc != 2)
    return 1;
 
  if (init_sdl())
  {
    return 1;
  }
 
  fd = open(argv[1], O_RDONLY);
  if(fd < 0) {
    perror("multipoint: error opening");
    return 1;
  }

/*print_device_info(fd);
print_events(fd);*/


  double min[2];
  double max[2];

	int abs[6] = {0};
	int k;

	ioctl(fd, EVIOCGABS(ABS_X), abs);

  min[0] = abs[1];
  max[0] = abs[2];

	ioctl(fd, EVIOCGABS(ABS_Y), abs);
  min[1] = abs[1];
  max[1] = abs[2];

  int slot = 0;
  int changed = 0;

  struct input_event ev[64];
  int rc = ioctl(fd, EVIOCGRAB, (void*)1);
  if (rc)
  {
    perror ("impossible de réserver la tablette\n");
    return 1;
  }

  while (1)
  {
    rd = read(fd, ev, sizeof(struct input_event) * 64);

    if (rd < (int) sizeof(struct input_event))
    {
			  perror("multipoint: error reading");
			  return 1;
	  }

    unsigned i;
    for (i = 0; i < rd / sizeof(struct input_event); i++)
    {
      if (ev[i].type == EV_ABS)
      {
        changed = 1;
        if (ev[i].code == ABS_MT_SLOT)
          slot = ev[i].value;
        if (ev[i].code == ABS_MT_POSITION_X && slot < 10)
        {
          appui[slot] = 1;
          coord[slot][0] = (double) (ev[i].value - min[0]) / (max[0] - min[0]);
        }
        if (ev[i].code == ABS_MT_POSITION_Y && slot < 10)
        {
          appui[slot] = 1;
          coord[slot][1] = (double) (ev[i].value - min[1]) / (max[1] - min[1]);
        }
        if (ev[i].code == ABS_MT_TRACKING_ID && ev[i].value < 0 && slot < 10)
        {
          appui[slot] = 0;
        }
      }
    }
/*    if (changed)
    {
      for (i = 0; i < 10; i++)
      {
        if (appui[i])
        {
          printf ("(%8f, %8f) ", coord[i][0], coord[i][1]);
        }
        else
        {
          printf ("                     ");
        }
      }
      printf("\n");
    }*/
    
  }
  close (fd);

  return 0;
}
