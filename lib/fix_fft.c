/* fix_fft.c - Fixed-point in-place Fast Fourier Transform  */
/*
  All data are fixed-point short integers, in which -32768
  to +32768 represent -1.0 to +1.0 respectively. Integer
  arithmetic is used for speed, instead of the more natural
  floating-point.

  For the forward FFT (time -> freq), fixed scaling is
  performed to prevent arithmetic overflow, and to map a 0dB
  sine/cosine wave (i.e. amplitude = 32767) to two -6dB freq
  coefficients. The return value is always 0.

  For the inverse FFT (freq -> time), fixed scaling cannot be
  done, as two 0dB coefficients would sum to a peak amplitude
  of 64K, overflowing the 32k range of the fixed-point integers.
  Thus, the fix_fft() routine performs variable scaling, and
  returns a value which is the number of bits LEFT by which
  the output must be shifted to get the actual amplitude
  (i.e. if fix_fft() returns 3, each value of fr[] and fi[]
  must be multiplied by 8 (2**3) for proper scaling.
  Clearly, this cannot be done within fixed-point short
  integers. In practice, if the result is to be used as a
  filter, the scale_shift can usually be ignored, as the
  result will be approximately correctly normalized as is.

  Written by:  Tom Roberts  11/8/89
  Made portable:  Malcolm Slaney 12/15/94 malcolm@interval.com
  Enhanced:  Dimitrios P. Bouras  14 Jun 2006 dbouras@ieee.org
*/

#define N_WAVE      2048    /* full length of Sinewave[] */
#define LOG2_N_WAVE 11      /* log2(N_WAVE) */

/*
  Henceforth "short" implies 16-bit word. If this is not
  the case in your architecture, please replace "short"
  with a type definition which *is* a 16-bit word.
*/

#include "sineW.h"

/*
  FIX_MPY() - fixed-point multiplication & scaling.
  Substitute inline assembly for hardware-specific
  optimization suited to a particluar DSP processor.
  Scaling ensures that result remains 16-bit.
*/
short FIX_MPY(short a, short b)
{
  /* shift right one less bit (i.e. 15-1) */
  int c = ((int)a * (int)b) >> 14;
  /* last bit shifted out = rounding-bit */
  b = c & 0x01;
  /* last shift + rounding bit */
  a = (c >> 1) + b;
  return a;
}

typedef struct reorderArg{
  short * fr;
  short * fi;
}reorderArg_t;

/*
 *
 */
void parallel_reorderData(reorderArg_t * p)
{
  short m,l,tr,ti;
  short n = N_WAVE;
  //nn will receive my_last to proccesing multicore
  short  nn = n - 1;
  short r0_t[] = {1,131,271,423,591,783,1008,1304};
  short re_t[] = {130,270,422,590,782,1007,1303,1983};
  short mr_t[] = {0,520,900,812,914,902,1982,1861};
  
  short r0 = r0_t[rt_core_id()];
  short re = re_t[rt_core_id()];
  short mr = mr_t[rt_core_id()];

  for (m=r0; m<=re; ++m) {
    l = n;
    do {
      l >>= 1;
    } while (mr+l > nn);
    mr = (mr & (l-1)) + l;

    if (mr <= m)
      continue;
    tr = p->fr[m];
    p->fr[m] = p->fr[mr];
    p->fr[mr] = tr;
    ti = p->fi[m];
    p->fi[m] = p->fi[mr];
    p->fi[mr] = ti;
  }
}


typedef struct BflyArg{
  short * fr;
  short * fi;
  short l;
  short k;
  short istep;
}BflyArg_t;

//(fr,fi,l,k,istep);
int gBflyStep(BflyArg_t * Arg)
{
  short m, wr, wi, tr, ti, qr, qi, j, i, i0, ie;
  short l = Arg->l;
  short k = Arg->k;
  short istep = Arg->istep;
  m = (rt_core_id()/(rt_nb_pe()/l))&0xf;
  i0 = l*rt_core_id()*N_WAVE/rt_nb_pe()-((N_WAVE-1)*m);
  ie = l*(rt_core_id()+1)*N_WAVE/rt_nb_pe()-((N_WAVE)*m);
  j = m << k;

  //printf("here! %d\n", rt_core_id());
  // 0 <= j < N_WAVE/2 
  wr = SineW[j+N_WAVE/4]>>1;
  wi = -SineW[j]>>1;
  for (i=i0; i<ie; i+=istep) {
    j = i + l;
    tr = FIX_MPY(wr,Arg->fr[j]) - FIX_MPY(wi,Arg->fi[j]);
    ti = FIX_MPY(wr,Arg->fi[j]) + FIX_MPY(wi,Arg->fr[j]);
    qr = Arg->fr[i]>>1;
    qi = Arg->fi[i]>>1;
    Arg->fr[j] = qr - tr;
    Arg->fi[j] = qi - ti;
    Arg->fr[i] = qr + tr;
    Arg->fi[i] = qi + ti;
  }
  rt_team_barrier();
//  printf("here! %d\n", rt_core_id());
  return 0;
}

int gTeamStep(BflyArg_t * Arg)
{
  short m, wr, wi, tr, ti, qr, qi, j, i, i0, ie, m0, me;
  short l = Arg->l;
  short k = Arg->k;
  short istep = Arg->istep;
  m0 = rt_core_id()*(l/rt_nb_pe());
  me = (rt_core_id()+1)*(l/rt_nb_pe());
  //printf("here!\n");
  for (m=m0; m<me; ++m) {
    j = m << k;
// 0 <= j < N_WAVE/2 
    wr = SineW[j+N_WAVE/4]>>1;
    wi = -SineW[j]>>1;
    for (i=m; i<N_WAVE; i+=istep) {
      j = i + l;
      tr = FIX_MPY(wr,Arg->fr[j]) - FIX_MPY(wi,Arg->fi[j]);
      ti = FIX_MPY(wr,Arg->fi[j]) + FIX_MPY(wi,Arg->fr[j]);
      qr = Arg->fr[i]>>1;
      qi = Arg->fi[i]>>1;
      Arg->fr[j] = qr - tr;
      Arg->fi[j] = qi - ti;
      Arg->fr[i] = qr + tr;
      Arg->fi[i] = qi + ti;
    }
  }
  rt_team_barrier();
  return 0;
}

/*
  fix_fft() - perform forward/inverse fast Fourier transform.
  fr[n],fi[n] are real and imaginary arrays, both INPUT AND
  RESULT (in-place FFT), with 0 <= n < 2**m; set inverse to
  0 for forward transform (FFT), or 1 for iFFT.
*/

int parallel_fix_fft(short fr[], short fi[], short m)
{
  int mr, nn, i, j, l, k, istep, n;
  short qr, qi, tr, ti, wr, wi;
  
//  struct BflyArg_t;
//  printf("Declarating Args to fork\n");
  BflyArg_t Arg;
  Arg.fr = fr;
  Arg.fi = fi;
  
  reorderArg_t rArg;
  rArg.fr = fr;
  rArg.fi = fi;
 
  n = 1 << m;

  // max FFT size = N_WAVE 
  if (n > N_WAVE)
    return -1;

  //decimation in time - re-order data 
  rt_team_fork(rt_nb_pe(),(void *)parallel_reorderData,(void *)&rArg);
  //reorderData(n,fr,fi);
  l = 1;
  k = LOG2_N_WAVE-1;
  while (l < n) {
    istep = l << 1;
    Arg.l = l;
    Arg.k = k;
    Arg.istep = istep;
    if(l<rt_nb_pe()){
    //rt_team_fork(number_of_cores,function_entry, argument_of_the_function)
    //number of cores: rt_nb_pe()
      rt_team_fork(rt_nb_pe(),(void *)gBflyStep,(void *)&Arg);//(fr,fi,l,k,istep);
    }else{
      rt_team_fork(rt_nb_pe(),(void *)gTeamStep,(void *)&Arg);//(fr,fi,l,k,istep); 
    }
//    rt_team_barrier();
    --k;
    l = istep;
 //   printf("End stage l= %d istep= %d",l,istep);
  }
  return 0;
}

void reorderData(short n, short fr[], short fi[])
{
  short m,l,tr,ti;
  short mr=0;
  //nn will receive my_last to proccesing multicore
  short  nn = n - 1;
  
  for (m=1; m<=nn; ++m) {
    l = n;
    do {
      l >>= 1;
    } while (mr+l > nn);
    mr = (mr & (l-1)) + l;

    if (mr <= m)
      continue;
    tr = fr[m];
    fr[m] = fr[mr];
    fr[mr] = tr;
    ti = fi[m];
    fi[m] = fi[mr];
    fi[mr] = ti;
  }
}

/*
  fix_fft() - perform forward/inverse fast Fourier transform.
  fr[n],fi[n] are real and imaginary arrays, both INPUT AND
  RESULT (in-place FFT), with 0 <= n < 2**m; set inverse to
  0 for forward transform (FFT), or 1 for iFFT.
*/
int fix_fft(short fr[], short fi[], short m, short inverse)
{
  int mr, nn, i, j, l, k, istep, n, scale, shift;
  short qr, qi, tr, ti, wr, wi;
  
  n = 1 << m;

  /* max FFT size = N_WAVE */
  if (n > N_WAVE)
    return -1;
  
  scale = 0;

  /* decimation in time - re-order data */
  reorderData(n, fr, fi);

  l = 1;
  k = LOG2_N_WAVE-1;
  while (l < n) {
    if (inverse) {
      /* variable scaling, depending upon data */
      shift = 0;
      for (i=0; i<n; ++i) {
        j = fr[i];
        if (j < 0)
          j = -j;
        m = fi[i];
        if (m < 0)
          m = -m;
        if (j > 16383 || m > 16383) {
          shift = 1;
          break;
        }
      }
      if (shift)
        ++scale;
    } else {
      /*
        fixed scaling, for proper normalization --
        there will be log2(n) passes, so this results
        in an overall factor of 1/n, distributed to
        maximize arithmetic accuracy.
      */
      shift = 1;
    }
    /*
      it may not be obvious, but the shift will be
      performed on each data point exactly once,
      during this pass.
    */
    istep = l << 1;
    for (m=0; m<l; ++m) {
      j = m << k;
  //  0 <= j < N_WAVE/2 
      wr = SineW[j+N_WAVE/4];
      wi = -SineW[j];
      if (inverse)
        wi = -wi;
      if (shift) {
        wr >>= 1;
        wi >>= 1;
      }
      for (i=m; i<n; i+=istep) {
        j = i + l;
        tr = FIX_MPY(wr,fr[j]) - FIX_MPY(wi,fi[j]);
        ti = FIX_MPY(wr,fi[j]) + FIX_MPY(wi,fr[j]);
        qr = fr[i];
        qi = fi[i];
        if (shift) {
          qr >>= 1;
          qi >>= 1;
        }
        fr[j] = qr - tr;
        fi[j] = qi - ti;
        fr[i] = qr + tr;
        fi[i] = qi + ti;
      }
    }
    --k;
    l = istep;
  }
  return scale;
}

/*
  fix_fftr() - forward/inverse FFT on array of real numbers.
  Real FFT/iFFT using half-size complex FFT by distributing
  even/odd samples into real/imaginary arrays respectively.
  In order to save data space (i.e. to avoid two arrays, one
  for real, one for imaginary samples), we proceed in the
  following two steps: a) samples are rearranged in the real
  array so that all even samples are in places 0-(N/2-1) and
  all imaginary samples in places (N/2)-(N-1), and b) fix_fft
  is called with fr and fi pointing to index 0 and index N/2
  respectively in the original array. The above guarantees
  that fix_fft "sees" consecutive real samples as alternating
  real and imaginary samples in the complex array.
*/
int fix_fftr(short f[], int m, int inverse)
{
  int i, N = 1<<(m-1), scale = 0;
  short tt, *fr=f, *fi=&f[N];

  if (inverse)
    scale = fix_fft(fi, fr, m-1, inverse);
  for (i=1; i<N; i+=2) {
    tt = f[N+i-1];
    f[N+i-1] = f[i];
    f[i] = tt;
  }
  if (! inverse)
    scale = fix_fft(fi, fr, m-1, inverse);
  return scale;
}
