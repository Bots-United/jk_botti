#ifndef NEURALNET_H
#define NEURALNET_H

#include "new_baseclass.h"

/******************************************************************* CNeuron */
class CNeuron : public class_new_baseclass
{
public:
   int m_num_inputs;
   double *m_weights;

   CNeuron(void): m_num_inputs(0), m_weights(NULL) {}
   CNeuron(int num_inputs, double in_weights[]);

   int get_num_inputs(void) const { return m_num_inputs; }

   static int calc_needed_weights(int num_inputs);
};

/************************************************************** CNeuronLayer */
class CNeuronLayer : public class_new_baseclass
{
public:

   int m_num_neurons;
   CNeuron *m_neurons;

   CNeuronLayer(void): m_num_neurons(0), m_neurons(NULL) {}
   CNeuronLayer(int num_neurons, int num_inputs_per_neuron, CNeuron in_neurons[], double in_weights[]);

   int get_num_neurons(void) const { return m_num_neurons; }

   static int calc_needed_weights(int num_neurons, int num_inputs_per_neuron);
};

/**************************************************************** CNeuralNet */
class CNeuralNet : public class_new_baseclass
{
private:
   int m_num_inputs;
   int m_num_outputs;
   int m_num_hidden;
   int m_num_neurons_per_hidden;
   int m_widest_weight_array;
   int m_widest_layer;

   double m_bias;

   int m_num_layers;
   CNeuronLayer *m_layers;

   int m_num_weights;
   double *m_weights;

   CNeuron *m_neurons;

   double *m_run_tmp1;
   double *m_run_tmp2;

   double *run_internal(const double orig_inputs[], double *ptr1, double *ptr2) const;

   // prevent copying (pointers to internal buffers)
   CNeuralNet(const CNeuralNet &);
   CNeuralNet &operator=(const CNeuralNet &);

public:
   CNeuralNet(int num_inputs, int num_outputs, int num_hidden, int num_neurons_per_hidden);
   ~CNeuralNet(void);

   int get_num_weights(void) const { return m_num_weights; }
   int get_num_inputs(void) const { return m_num_inputs; }
   int get_num_outputs(void) const { return m_num_outputs; }

   void reset_weights_random(void);
   double *get_weights(double weights[]) const;
   void put_weights(double weights[]);

   double *run(const double inputs[], double outputs[]) const;
   double *run(const double inputs[], double outputs[], const double scales[]) const;

   void print(void) const;
};

#endif // NEURALNET_H
