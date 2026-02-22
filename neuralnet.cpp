#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <math.h>
#include <limits.h>

#include "neuralnet.h"
#include "geneticalg.h"

// Manual branch optimization for GCC 3.0.0 and newer
#if !defined(__GNUC__) || __GNUC__ < 3
   #define likely(x) (x)
   #define unlikely(x) (x)
#else
   #define likely(x) __builtin_expect((long int)!!(x), true)
   #define unlikely(x) __builtin_expect((long int)!!(x), false)
#endif

extern void fast_random_seed(unsigned int seed);
extern int RANDOM_LONG2(int lLow, int lHigh);
extern float RANDOM_FLOAT2(float flLow, float flHigh);

static int get_random_int(int x,int y)
{
   return RANDOM_LONG2(x, y);
}

// return random value in range 0 < n < 1
static double get_random(void)
{
   return RANDOM_FLOAT2(0.0, 1.0);
}

// return random value in range -1 < n < 1
static double get_random_weight(void)
{
   return get_random() - get_random();
}

/******************************************************************* CNeuron */
CNeuron::CNeuron(int num_inputs, double in_weights[]):
   m_num_inputs(num_inputs + 1),
   m_weights(in_weights)
{
}

// one extra slot needed for bias
// returns -1 on overflow
int CNeuron::calc_needed_weights(int num_inputs)
{
   if (num_inputs < 0 || num_inputs >= INT_MAX)
      return -1;
   return num_inputs + 1;
}

/************************************************************** CNeuronLayer */
CNeuronLayer::CNeuronLayer(int num_neurons, int num_inputs_per_neuron, CNeuron in_neurons[], double in_weights[]):
   m_num_neurons(num_neurons),
   m_neurons(in_neurons)
{
   int i, weight_pos;

   weight_pos = 0;
   for (i = 0; i < m_num_neurons; i++) {
      m_neurons[i] = CNeuron(num_inputs_per_neuron, &in_weights[weight_pos]);
      weight_pos += CNeuron::calc_needed_weights(num_inputs_per_neuron);
   }
}

// returns -1 on overflow
int CNeuronLayer::calc_needed_weights(int num_neurons, int num_inputs_per_neuron)
{
   int per_neuron = CNeuron::calc_needed_weights(num_inputs_per_neuron);
   if (per_neuron < 0 || num_neurons <= 0 || per_neuron > INT_MAX / num_neurons)
      return -1;
   return per_neuron * num_neurons;
}

/**************************************************************** CNeuralNet */
CNeuralNet::CNeuralNet(int num_inputs, int num_outputs, int num_hidden, int num_neurons_per_hidden):
   m_num_inputs(num_inputs),
   m_num_outputs(num_outputs),
   m_num_hidden(num_hidden),
   m_num_neurons_per_hidden(num_neurons_per_hidden),
   m_widest_weight_array(0),
   m_widest_layer(0),
   m_bias(-1.0),
   m_num_layers(num_hidden + 1), // hidden + output
   m_layers(NULL),
   m_num_weights(0),
   m_weights(NULL),
   m_neurons(NULL),
   m_run_tmp1(NULL),
   m_run_tmp2(NULL)
{
   int i, j, weight_pos, neuron_pos, total_neurons;

   if (m_num_layers == 0)
      return;

   // calculated number of needed weights in neural network
   if (m_num_layers == 1) {
      m_num_weights = CNeuronLayer::calc_needed_weights(m_num_outputs, m_num_inputs);
   } else {
      int w1 = CNeuronLayer::calc_needed_weights(m_num_neurons_per_hidden, m_num_inputs);
      int w2 = CNeuronLayer::calc_needed_weights(m_num_neurons_per_hidden, m_num_neurons_per_hidden);
      int w3 = CNeuronLayer::calc_needed_weights(m_num_outputs, m_num_neurons_per_hidden);
      int inner_layers = m_num_layers - 2;
      int w2_total;

      if (w1 < 0 || w2 < 0 || w3 < 0)
         return;
      if (inner_layers > 0 && w2 > INT_MAX / inner_layers)
         return;
      w2_total = w2 * inner_layers;
      if (w1 > INT_MAX - w2_total || w1 + w2_total > INT_MAX - w3)
         return;
      m_num_weights = w1 + w2_total + w3;
   }
   if (m_num_weights < 0)
      return;

   // create network wide weight array
   m_weights = (double *)calloc(1, sizeof(double) * m_num_weights);
   if (!m_weights)
      return;
   reset_weights_random();

   // create network wide neuron array (check for overflow)
   if (m_num_hidden > 0 && m_num_neurons_per_hidden > INT_MAX / m_num_hidden)
      return;
   total_neurons = m_num_neurons_per_hidden * m_num_hidden;
   if (m_num_outputs > INT_MAX - total_neurons)
      return;
   total_neurons += m_num_outputs;
   m_neurons = (CNeuron *)calloc(1, sizeof(CNeuron) * total_neurons);
   if (!m_neurons)
      return;

   // create neuron layers
   m_layers = new CNeuronLayer[m_num_layers];

   if (m_num_layers == 1) {
      // create output layer
      m_layers[0] = CNeuronLayer(m_num_outputs, m_num_inputs, m_neurons, m_weights);
   } else {
      // create first hidden layer
      m_layers[0] = CNeuronLayer(m_num_neurons_per_hidden, m_num_inputs, m_neurons, m_weights);
      weight_pos = CNeuronLayer::calc_needed_weights(m_num_neurons_per_hidden, m_num_inputs);
      neuron_pos = m_num_neurons_per_hidden;

      // create inner hidden layers
      for (i = 1; i < m_num_layers - 1; i++) {
         m_layers[i] = CNeuronLayer(m_num_neurons_per_hidden, m_num_neurons_per_hidden, &m_neurons[neuron_pos], &m_weights[weight_pos]);
         weight_pos += CNeuronLayer::calc_needed_weights(m_num_neurons_per_hidden, m_num_neurons_per_hidden);
         neuron_pos += m_num_neurons_per_hidden;
      }

      // create output layer
      m_layers[m_num_layers - 1] = CNeuronLayer(m_num_outputs, m_num_neurons_per_hidden, &m_neurons[neuron_pos], &m_weights[weight_pos]);
   }

   // get widest layer and weight array
   m_widest_weight_array = 0;
   m_widest_layer = 0;
   for (i = 0; i < m_num_layers; i++) {
      if (m_widest_layer < m_layers[i].get_num_neurons())
         m_widest_layer = m_layers[i].get_num_neurons();
      for (j = 0; j < m_layers[i].get_num_neurons(); j++)
         if (m_widest_weight_array < m_layers[i].m_neurons[j].get_num_inputs())
            m_widest_weight_array = m_layers[i].m_neurons[j].get_num_inputs();
   }

   // allocate temporary buffers for run()
   m_run_tmp1 = (double *)calloc(m_widest_layer, sizeof(double));
   m_run_tmp2 = (double *)calloc(m_widest_layer, sizeof(double));
}

// destructor, release memory
CNeuralNet::~CNeuralNet(void)
{
   if (m_weights) {
      free(m_weights);
      m_weights = NULL;
   }

   if (m_layers) {
      delete[] m_layers;
      m_layers = NULL;
   }

   if (m_neurons) {
      free(m_neurons);
      m_neurons = NULL;
   }

   if (m_run_tmp1) {
      free(m_run_tmp1);
      m_run_tmp1 = NULL;
   }

   if (m_run_tmp2) {
      free(m_run_tmp2);
      m_run_tmp2 = NULL;
   }
}

// reset neuron weights to random
void CNeuralNet::reset_weights_random(void)
{
   int i;

   for (i = 0; i < m_num_weights; i++)
      m_weights[i] = get_random_weight();
}

// weights must have array size of m_num_weights
double *CNeuralNet::get_weights(double weights[]) const
{
   int i;

   for (i = 0; i < m_num_weights; i++)
      weights[i] = m_weights[i];

   return weights;
}

// weights must have array size of m_num_weights, allocated by caller
void CNeuralNet::put_weights(double weights[])
{
   int i;

   for (i = 0; i < m_num_weights; i++)
      m_weights[i] = weights[i];
}

static double expanded_sigmoid(double activation, double response = 1.0)
{
   double s;

   // input activation is -1..1, convert to 0..1
   activation = (activation + 1) / 2;

   s = 1 / ( 1 + exp(-activation / response));

   // s is 0..1, convert to -1..1
   return (s * 2) - 1;
}

double *CNeuralNet::run(const double inputs[], double outputs[]) const
{
   return run(inputs, outputs, NULL);
}

double *CNeuralNet::run(const double inputs[], double outputs[], const double scales[]) const
{
   int i;
   double *out;

   if (m_num_layers < 1)
      return NULL;

   out = run_internal(inputs, m_run_tmp1, m_run_tmp2);

   if (scales)
      for (i = 0; i < m_num_outputs; i++)
         outputs[i] = out[i] * scales[i];
   else
      for (i = 0; i < m_num_outputs; i++)
         outputs[i] = out[i];

   return outputs;
}

// run neural network, inputs must have array size of m_num_inputs
// and output must have array size of m_num_outputs
// ptr1 and ptr2 are temporary buffers
double *CNeuralNet::run_internal(const double orig_inputs[], double *ptr1, double *ptr2) const
{
   int i, j, k, in_size, out_size = 0;
   double *outputs = NULL;
   const double *inputs;
   bool ptr1_used = false, ptr2_used = false;

   inputs = orig_inputs;
   in_size = m_num_inputs;

   // walk through all layers
   for (i = 0; i < m_num_layers; i++) {
      const CNeuronLayer &cur_layer = m_layers[i];

      if (likely(i > 0)) {
         // free old input slot
         ptr1_used = !(inputs == ptr1) & ptr1_used;
         ptr2_used = !(inputs == ptr2) & ptr2_used;

         inputs = outputs;
         in_size = out_size;
      }

      out_size = cur_layer.get_num_neurons();

      // get memory slot for output array
      if (!ptr1_used) {
         outputs = ptr1;
         ptr1_used = true;
      } else {
         outputs = ptr2;
         ptr2_used = true;
      }

      // walk through all neurons in current layer
      for (j = 0; j < cur_layer.get_num_neurons(); j++) {
         const CNeuron &cur_neuron = cur_layer.m_neurons[j];
         double sum = 0;

         // for each weight
         for (k = 0; k < cur_neuron.get_num_inputs() - 1; k++)
            //sum the weights x inputs
            sum += cur_neuron.m_weights[k] * inputs[k];

         // add bias
         sum += cur_neuron.m_weights[cur_neuron.get_num_inputs() - 1] * m_bias;

         outputs[j] = expanded_sigmoid(sum);
      }
   }

   return outputs;
}

void CNeuralNet::print(void) const
{
   int i, j, k;

   for (i = 0; i < m_num_layers; i++) {
      printf("layer [%i] neurons[%i]:\n", i, m_layers[i].get_num_neurons());

      for (j = 0; j < m_layers[i].get_num_neurons(); j++) {
         printf("  neuron [%i:%i] inputs[%i]:\n", i, j, m_layers[i].m_neurons[j].get_num_inputs());

         printf("    ");
         for (k = 0; k < m_layers[i].m_neurons[j].get_num_inputs(); k++) {
            printf("[%f] ", m_layers[i].m_neurons[j].m_weights[k]);
         }
         printf("\n");
      }
   }
}
