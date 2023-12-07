/****************************************************************************
*   Generated by ACUITY 6.0.12
*   Match ovxlib 1.1.34
*
*   Neural Network appliction network definition source file
****************************************************************************/
/*-------------------------------------------
                   Includes
 -------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "vsi_nn_pub.h"

#include "vnn_global.h"
#include "vnn_policy89.h"

/*-------------------------------------------
                   Macros
 -------------------------------------------*/

#define NEW_VXNODE(_node, _type, _in, _out, _uid) do {\
        _node = vsi_nn_AddNode( graph, _type, _in, _out, NULL );\
        if( NULL == _node ) {\
            goto error;\
        }\
        _node->uid = (uint32_t)_uid;\
    } while(0)

#define NEW_VIRTUAL_TENSOR(_id, _attr, _dtype) do {\
        memset( _attr.size, 0, VSI_NN_MAX_DIM_NUM * sizeof(vsi_size_t));\
        _attr.dim_num = VSI_NN_DIM_AUTO;\
        _attr.vtl = !VNN_APP_DEBUG;\
        _attr.is_const = FALSE;\
        _attr.dtype.vx_type = _dtype;\
        _id = vsi_nn_AddTensor( graph, VSI_NN_TENSOR_ID_AUTO,\
                & _attr, NULL );\
        if( VSI_NN_TENSOR_ID_NA == _id ) {\
            goto error;\
        }\
    } while(0)

// Set const tensor dims out of this macro.
#define NEW_CONST_TENSOR(_id, _attr, _dtype, _ofst, _size) do {\
        data = load_data( fp, _ofst, _size  );\
        _attr.vtl = FALSE;\
        _attr.is_const = TRUE;\
        _attr.dtype.vx_type = _dtype;\
        _id = vsi_nn_AddTensor( graph, VSI_NN_TENSOR_ID_AUTO,\
                & _attr, data );\
        free( data );\
        if( VSI_NN_TENSOR_ID_NA == _id ) {\
            goto error;\
        }\
    } while(0)

// Set generic tensor dims out of this macro.
#define NEW_NORM_TENSOR(_id, _attr, _dtype) do {\
        _attr.vtl = FALSE;\
        _attr.is_const = FALSE;\
        _attr.dtype.vx_type = _dtype;\
        _id = vsi_nn_AddTensor( graph, VSI_NN_TENSOR_ID_AUTO,\
                & _attr, NULL );\
        if( VSI_NN_TENSOR_ID_NA == _id ) {\
            goto error;\
        }\
    } while(0)

// Set generic tensor dims out of this macro.
#define NEW_NORM_TENSOR_FROM_HANDLE(_id, _attr, _dtype) do {\
        _attr.vtl = FALSE;\
        _attr.is_const = FALSE;\
        _attr.dtype.vx_type = _dtype;\
        _id = vsi_nn_AddTensorFromHandle( graph, VSI_NN_TENSOR_ID_AUTO,\
                & _attr, NULL );\
        if( VSI_NN_TENSOR_ID_NA == _id ) {\
            goto error;\
        }\
    } while(0)

#define NET_NODE_NUM            (8)
#define NET_NORM_TENSOR_NUM     (2)
#define NET_CONST_TENSOR_NUM    (8)
#define NET_VIRTUAL_TENSOR_NUM  (8)
#define NET_TOTAL_TENSOR_NUM    (NET_NORM_TENSOR_NUM + NET_CONST_TENSOR_NUM + NET_VIRTUAL_TENSOR_NUM)

/*-------------------------------------------
               Local Variables
 -------------------------------------------*/

/*-------------------------------------------
                  Functions
 -------------------------------------------*/
static uint8_t* load_data
    (
    FILE  * fp,
    size_t  ofst,
    size_t  sz
    )
{
    uint8_t* data;
    int32_t ret;
    data = NULL;
    if( NULL == fp )
    {
        return NULL;
    }

    ret = fseek(fp, ofst, SEEK_SET);
    if (ret != 0)
    {
        VSILOGE("blob seek failure.");
        return NULL;
    }

    data = (uint8_t*)malloc(sz);
    if (data == NULL)
    {
        VSILOGE("buffer malloc failure.");
        return NULL;
    }
    ret = fread(data, 1, sz, fp);
    return data;
} /* load_data() */

vsi_nn_graph_t * vnn_CreatePolicy89
    (
    const char * data_file_name,
    vsi_nn_context_t in_ctx,
    const vsi_nn_preprocess_map_element_t * pre_process_map,
    uint32_t pre_process_map_count,
    const vsi_nn_postprocess_map_element_t * post_process_map,
    uint32_t post_process_map_count
    )
{
    uint32_t                _infinity = VSI_NN_FLOAT32_INF;
    vsi_status              status;
    vsi_bool                release_ctx;
    vsi_nn_context_t        ctx;
    vsi_nn_graph_t *        graph;
    vsi_nn_node_t *         node[NET_NODE_NUM];
    vsi_nn_tensor_id_t      norm_tensor[NET_NORM_TENSOR_NUM];
    vsi_nn_tensor_id_t      const_tensor[NET_CONST_TENSOR_NUM];
    vsi_nn_tensor_attr_t    attr;
    FILE *                  fp;
    uint8_t *               data;
    uint32_t                i = 0;
    char *                  use_img_process_s;
    int32_t                 enable_pre_post_process = 0;
    vsi_bool                sort = FALSE;
    vsi_bool                inference_with_nbg = FALSE;
    char*                   pos = NULL;





    (void)(_infinity);
    ctx = NULL;
    graph = NULL;
    status = VSI_FAILURE;
    memset( &attr, 0, sizeof( attr ) );
    memset( &node, 0, sizeof( vsi_nn_node_t * ) * NET_NODE_NUM );

    fp = fopen( data_file_name, "rb" );
    if( NULL == fp )
    {
        VSILOGE( "Open file %s failed.", data_file_name );
        goto error;
    }

    pos = strstr(data_file_name, ".nb");
    if( pos && strcmp(pos, ".nb") == 0 )
    {
        inference_with_nbg = TRUE;
    }

    if( NULL == in_ctx )
    {
        ctx = vsi_nn_CreateContext();
    }
    else
    {
        ctx = in_ctx;
    }

    use_img_process_s = getenv( "VSI_USE_IMAGE_PROCESS" );
    if( use_img_process_s )
    {
        enable_pre_post_process = atoi(use_img_process_s);
    }

    graph = vsi_nn_CreateGraph( ctx, NET_TOTAL_TENSOR_NUM, NET_NODE_NUM );
    if( NULL == graph )
    {
        VSILOGE( "Create graph fail." );
        goto error;
    }
    vsi_nn_SetGraphVersion( graph, VNN_VERSION_MAJOR, VNN_VERSION_MINOR, VNN_VERSION_PATCH );
    vsi_nn_SetGraphInputs( graph, NULL, 1 );
    vsi_nn_SetGraphOutputs( graph, NULL, 1 );

/*-----------------------------------------
  Register client ops
 -----------------------------------------*/


/*-----------------------------------------
  Node definitions
 -----------------------------------------*/
    if( !inference_with_nbg )
    {

    /*-----------------------------------------
      lid       - Cast_/Cast_9
      var       - node[0]
      name      - Cast_/Cast
      operation - cast
      input     - [23, 1]
      output    - [23, 1]
    -----------------------------------------*/
    NEW_VXNODE(node[0], VSI_NN_OP_DATACONVERT, 1, 1, 9);

    /*-----------------------------------------
      lid       - Gemm_/extractor2/policy_net/policy_net.0/Gemm_7
      var       - node[1]
      name      - Gemm_/extractor2/policy_net/policy_net.0/Gemm
      operation - fullconnect
      input     - [23, 1]
      filter    - [23, 256]
      output    - [256, 1]
    -----------------------------------------*/
    NEW_VXNODE(node[1], VSI_NN_OP_FCL, 3, 1, 7);
    node[1]->nn_param.fcl.weights = 256;
    node[1]->vx_param.overflow_policy = VX_CONVERT_POLICY_SATURATE;
    node[1]->vx_param.rounding_policy = VX_ROUND_POLICY_TO_NEAREST_EVEN;
    node[1]->vx_param.down_scale_size_rounding = VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_FLOOR;

    /*-----------------------------------------
      lid       - Relu_/extractor2/policy_net/policy_net.1/Relu_6
      var       - node[2]
      name      - Relu_/extractor2/policy_net/policy_net.1/Relu
      operation - relu
      input     - [256, 1]
      output    - [256, 1]
    -----------------------------------------*/
    NEW_VXNODE(node[2], VSI_NN_OP_RELU, 1, 1, 6);

    /*-----------------------------------------
      lid       - Gemm_/extractor2/policy_net/policy_net.2/Gemm_5
      var       - node[3]
      name      - Gemm_/extractor2/policy_net/policy_net.2/Gemm
      operation - fullconnect
      input     - [256, 1]
      filter    - [256, 256]
      output    - [256, 1]
    -----------------------------------------*/
    NEW_VXNODE(node[3], VSI_NN_OP_FCL, 3, 1, 5);
    node[3]->nn_param.fcl.weights = 256;
    node[3]->vx_param.overflow_policy = VX_CONVERT_POLICY_SATURATE;
    node[3]->vx_param.rounding_policy = VX_ROUND_POLICY_TO_NEAREST_EVEN;
    node[3]->vx_param.down_scale_size_rounding = VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_FLOOR;

    /*-----------------------------------------
      lid       - Relu_/extractor2/policy_net/policy_net.3/Relu_4
      var       - node[4]
      name      - Relu_/extractor2/policy_net/policy_net.3/Relu
      operation - relu
      input     - [256, 1]
      output    - [256, 1]
    -----------------------------------------*/
    NEW_VXNODE(node[4], VSI_NN_OP_RELU, 1, 1, 4);

    /*-----------------------------------------
      lid       - Gemm_/extractor2/policy_net/policy_net.4/Gemm_3
      var       - node[5]
      name      - Gemm_/extractor2/policy_net/policy_net.4/Gemm
      operation - fullconnect
      input     - [256, 1]
      filter    - [256, 256]
      output    - [256, 1]
    -----------------------------------------*/
    NEW_VXNODE(node[5], VSI_NN_OP_FCL, 3, 1, 3);
    node[5]->nn_param.fcl.weights = 256;
    node[5]->vx_param.overflow_policy = VX_CONVERT_POLICY_SATURATE;
    node[5]->vx_param.rounding_policy = VX_ROUND_POLICY_TO_NEAREST_EVEN;
    node[5]->vx_param.down_scale_size_rounding = VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_FLOOR;

    /*-----------------------------------------
      lid       - Relu_/extractor2/policy_net/policy_net.5/Relu_2
      var       - node[6]
      name      - Relu_/extractor2/policy_net/policy_net.5/Relu
      operation - relu
      input     - [256, 1]
      output    - [256, 1]
    -----------------------------------------*/
    NEW_VXNODE(node[6], VSI_NN_OP_RELU, 1, 1, 2);

    /*-----------------------------------------
      lid       - Gemm_/action_net/Gemm_1
      var       - node[7]
      name      - Gemm_/action_net/Gemm
      operation - fullconnect
      input     - [256, 1]
      filter    - [256, 4]
      output    - [4, 1]
    -----------------------------------------*/
    NEW_VXNODE(node[7], VSI_NN_OP_FCL, 3, 1, 1);
    node[7]->nn_param.fcl.weights = 4;
    node[7]->vx_param.overflow_policy = VX_CONVERT_POLICY_SATURATE;
    node[7]->vx_param.rounding_policy = VX_ROUND_POLICY_TO_NEAREST_EVEN;
    node[7]->vx_param.down_scale_size_rounding = VX_CONVOLUTIONAL_NETWORK_DS_SIZE_ROUNDING_FLOOR;

    }
    else
    {
    NEW_VXNODE(node[0], VSI_NN_OP_NBG, 1, 1, 0);
    node[0]->nn_param.nbg.type = VSI_NN_NBG_FILE;
    node[0]->nn_param.nbg.url = data_file_name;

    }

/*-----------------------------------------
  Tensor initialize
 -----------------------------------------*/
    attr.dtype.fmt = VSI_NN_DIM_FMT_NCHW;
    /* @attach_Gemm_/action_net/Gemm/out0_0:out0 */
    attr.size[0] = 4;
    attr.size[1] = 1;
    attr.dim_num = 2;
    attr.dtype.fl = 16;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_NORM_TENSOR(norm_tensor[0], attr, VSI_NN_TYPE_INT16);

    /* @input_10:out0 */
    attr.size[0] = 23;
    attr.size[1] = 1;
    attr.dim_num = 2;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_NONE;
    NEW_NORM_TENSOR(norm_tensor[1], attr, VSI_NN_TYPE_FLOAT16);



    if( !inference_with_nbg )
    {
    /* @Gemm_/extractor2/policy_net/policy_net.0/Gemm_7:weight */
    attr.size[0] = 23;
    attr.size[1] = 256;
    attr.dim_num = 2;
    attr.dtype.fl = 10;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_CONST_TENSOR(const_tensor[0], attr, VSI_NN_TYPE_INT16, 4128, 11776);

    /* @Gemm_/extractor2/policy_net/policy_net.0/Gemm_7:bias */
    attr.size[0] = 256;
    attr.dim_num = 1;
    attr.dtype.fl = 32;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_CONST_TENSOR(const_tensor[1], attr, VSI_NN_TYPE_INT64, 2080, 2048);

    /* @Gemm_/extractor2/policy_net/policy_net.2/Gemm_5:weight */
    attr.size[0] = 256;
    attr.size[1] = 256;
    attr.dim_num = 2;
    attr.dtype.fl = 14;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_CONST_TENSOR(const_tensor[2], attr, VSI_NN_TYPE_INT16, 17952, 131072);

    /* @Gemm_/extractor2/policy_net/policy_net.2/Gemm_5:bias */
    attr.size[0] = 256;
    attr.dim_num = 1;
    attr.dtype.fl = 32;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_CONST_TENSOR(const_tensor[3], attr, VSI_NN_TYPE_INT64, 15904, 2048);

    /* @Gemm_/extractor2/policy_net/policy_net.4/Gemm_3:weight */
    attr.size[0] = 256;
    attr.size[1] = 256;
    attr.dim_num = 2;
    attr.dtype.fl = 14;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_CONST_TENSOR(const_tensor[4], attr, VSI_NN_TYPE_INT16, 151072, 131072);

    /* @Gemm_/extractor2/policy_net/policy_net.4/Gemm_3:bias */
    attr.size[0] = 256;
    attr.dim_num = 1;
    attr.dtype.fl = 32;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_CONST_TENSOR(const_tensor[5], attr, VSI_NN_TYPE_INT64, 149024, 2048);

    /* @Gemm_/action_net/Gemm_1:weight */
    attr.size[0] = 256;
    attr.size[1] = 4;
    attr.dim_num = 2;
    attr.dtype.fl = 14;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_CONST_TENSOR(const_tensor[6], attr, VSI_NN_TYPE_INT16, 32, 2048);

    /* @Gemm_/action_net/Gemm_1:bias */
    attr.size[0] = 4;
    attr.dim_num = 1;
    attr.dtype.fl = 32;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_CONST_TENSOR(const_tensor[7], attr, VSI_NN_TYPE_INT64, 0, 32);



    /* @Cast_/Cast_9:out0 */
    attr.dtype.fl = 22;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_VIRTUAL_TENSOR(node[0]->output.tensors[0], attr, VSI_NN_TYPE_INT16);

    /* @Gemm_/extractor2/policy_net/policy_net.0/Gemm_7:out0 */
    attr.dtype.fl = 18;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_VIRTUAL_TENSOR(node[1]->output.tensors[0], attr, VSI_NN_TYPE_INT16);

    /* @Relu_/extractor2/policy_net/policy_net.1/Relu_6:out0 */
    attr.dtype.fl = 18;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_VIRTUAL_TENSOR(node[2]->output.tensors[0], attr, VSI_NN_TYPE_INT16);

    /* @Gemm_/extractor2/policy_net/policy_net.2/Gemm_5:out0 */
    attr.dtype.fl = 18;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_VIRTUAL_TENSOR(node[3]->output.tensors[0], attr, VSI_NN_TYPE_INT16);

    /* @Relu_/extractor2/policy_net/policy_net.3/Relu_4:out0 */
    attr.dtype.fl = 18;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_VIRTUAL_TENSOR(node[4]->output.tensors[0], attr, VSI_NN_TYPE_INT16);

    /* @Gemm_/extractor2/policy_net/policy_net.4/Gemm_3:out0 */
    attr.dtype.fl = 18;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_VIRTUAL_TENSOR(node[5]->output.tensors[0], attr, VSI_NN_TYPE_INT16);

    /* @Relu_/extractor2/policy_net/policy_net.5/Relu_2:out0 */
    attr.dtype.fl = 18;
    attr.dtype.qnt_type = VSI_NN_QNT_TYPE_DFP;
    NEW_VIRTUAL_TENSOR(node[6]->output.tensors[0], attr, VSI_NN_TYPE_INT16);



/*-----------------------------------------
  Connection initialize
 -----------------------------------------*/
    node[0]->input.tensors[0] = norm_tensor[1];
    node[7]->output.tensors[0] = norm_tensor[0];

    /* Cast_/Cast_9 */

    /* Gemm_/extractor2/policy_net/policy_net.0/Gemm_7 */
    node[1]->input.tensors[0] = node[0]->output.tensors[0];
    node[1]->input.tensors[1] = const_tensor[0]; /* data_weight */
    node[1]->input.tensors[2] = const_tensor[1]; /* data_bias */

    /* Relu_/extractor2/policy_net/policy_net.1/Relu_6 */
    node[2]->input.tensors[0] = node[1]->output.tensors[0];

    /* Gemm_/extractor2/policy_net/policy_net.2/Gemm_5 */
    node[3]->input.tensors[0] = node[2]->output.tensors[0];
    node[3]->input.tensors[1] = const_tensor[2]; /* data_weight */
    node[3]->input.tensors[2] = const_tensor[3]; /* data_bias */

    /* Relu_/extractor2/policy_net/policy_net.3/Relu_4 */
    node[4]->input.tensors[0] = node[3]->output.tensors[0];

    /* Gemm_/extractor2/policy_net/policy_net.4/Gemm_3 */
    node[5]->input.tensors[0] = node[4]->output.tensors[0];
    node[5]->input.tensors[1] = const_tensor[4]; /* data_weight */
    node[5]->input.tensors[2] = const_tensor[5]; /* data_bias */

    /* Relu_/extractor2/policy_net/policy_net.5/Relu_2 */
    node[6]->input.tensors[0] = node[5]->output.tensors[0];

    /* Gemm_/action_net/Gemm_1 */
    node[7]->input.tensors[0] = node[6]->output.tensors[0];
    node[7]->input.tensors[1] = const_tensor[6]; /* data_weight */
    node[7]->input.tensors[2] = const_tensor[7]; /* data_bias */


    }
    else
    {
    node[0]->output.tensors[0] = norm_tensor[0];
    node[0]->input.tensors[0] = norm_tensor[1];

    }
    graph->output.tensors[0] = norm_tensor[0];
    graph->input.tensors[0] = norm_tensor[1];


    if( enable_pre_post_process )
    {
        sort = TRUE;
        if( pre_process_map_count > 0 )
        {
            for( i = 0; i < pre_process_map_count; i++ )
            {
                status = vsi_nn_AddGraphPreProcess(graph, pre_process_map[i].graph_input_idx,
                                                   pre_process_map[i].preprocesses,
                                                   pre_process_map[i].preprocess_count);
                TEST_CHECK_STATUS( status, error );
            }
        }

        if( post_process_map_count > 0 )
        {
            for( i = 0; i < post_process_map_count; i++ )
            {
                 status = vsi_nn_AddGraphPostProcess(graph, post_process_map[i].graph_output_idx,
                                                     post_process_map[i].postprocesses,
                                                     post_process_map[i].postprocess_count);
                 TEST_CHECK_STATUS( status, error );
            }
        }
    }

    status = vsi_nn_SetupGraph( graph, sort );
    TEST_CHECK_STATUS( status, error );
    vsi_nn_DumpGraphToJson( graph );

    if( VSI_FAILURE == status )
    {
        goto error;
    }

    fclose( fp );

    return graph;

error:
    if( NULL != fp )
    {
        fclose( fp );
    }

    release_ctx = ( NULL == in_ctx );
    vsi_nn_DumpGraphToJson( graph );
    vnn_ReleasePolicy89( graph, release_ctx );

    return NULL;
} /* vsi_nn_CreatePolicy89() */

void vnn_ReleasePolicy89
    (
    vsi_nn_graph_t * graph,
    vsi_bool release_ctx
    )
{
    vsi_nn_context_t ctx;
    if( NULL != graph )
    {
        ctx = graph->ctx;
        vsi_nn_ReleaseGraph( &graph );

        /*-----------------------------------------
        Unregister client ops
        -----------------------------------------*/
        

        if( release_ctx )
        {
            vsi_nn_ReleaseContext( &ctx );
        }
    }
} /* vsi_nn_ReleasePolicy89() */

