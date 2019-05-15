#version 440 core

#define M_PI 3.14

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

uniform int len;

layout(binding = 0, rg32f) coherent uniform image1D input;
layout(binding = 1, r32i) coherent uniform iimage1D locks;

vec2 complexMult(vec2 a, vec2 b)
{
  return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

vec2 euler(float exponent)
{
  return vec2(cos(exponent), sin(exponent));
}

void main()
{
  int id = int(gl_GlobalInvocationID.x);
  vec2 mine = imageLoad(input, id).xy;
  int r = int(log2(len));

  imageAtomicExchange(locks, id, 0);
  memoryBarrier();
  barrier();

  for(int iter = 0; iter <= r; iter++)
  {
    int conj = id;

    if(iter == r) // reverse for final pass
    {
      conj = id;
      conj = bitfieldReverse(conj);
      conj = ((conj >> 1) & ~(1 << 31)) >> (31 - r);
    } else {
      conj = conj ^ (1 << (r - iter - 1));
    }

    bool isPartnerUpdated = false;
    bool partnerGotValue = false;
    vec2 other;
    vec2 result;

    while(!(isPartnerUpdated && partnerGotValue))
    {
      // ensure visibility of operations from other threads
      memoryBarrier();
      barrier();
      if(imageAtomicAdd(locks, conj, 0) >= iter*2) // partner has the value we require now
                                                   // geq instead of eq, since its possible for us to get the value and then proceed
                                                   // to update our lock before our conjugate checks
      {

        isPartnerUpdated = true; // update state
        other = imageLoad(input, conj).xy; // grab value
        imageAtomicAdd(locks, id, 1); // update our lock to iter*2+1

        if(r != iter)
        {
          int twiddle = id >> (r - iter - 1); // b0...bm... -> 0...0b0...bm
          twiddle = bitfieldReverse(twiddle); // bm...b0 0...0
          twiddle = ((twiddle >> 1) & ~(1 << 31)) >> (31 - r);

          if((id & (1 << (r - iter - 1))) != 0) // determine who is the coefficient of the twiddle
          {
            mine = other + complexMult(mine, euler(2*M_PI*twiddle/float(len)));
          } else {
            mine = mine + complexMult(other, euler(2*M_PI*twiddle/float(len)));
          }
        } else {
          mine = other;
        }
        // now we wait for our conjugate to get the old value so that we may safely update
      }
      if(imageAtomicAdd(locks, conj, 0) == iter*2 + 1) // our partner has received the value, update at will
      {

        partnerGotValue = true;
        imageStore(input, id, mine.xyxy);
        imageAtomicAdd(locks, id, 1); // update our lock to iter*2 + 2 = (iter+1)*2 and end the loop
      }
    }
  }
}
