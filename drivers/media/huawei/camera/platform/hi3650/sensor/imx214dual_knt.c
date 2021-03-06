


#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "hw_csi.h"
#include "../pmic/hw_pmic.h"


#define I2S(i) container_of(i, sensor_t, intf)

extern struct hw_csi_pad hw_csi_pad;
static hwsensor_vtbl_t s_imx214dual_vtbl;

static int imx214dual_config(hwsensor_intf_t* si, void  *argp);

static struct sensor_power_setting hw_imx214dual_power_setting[] = {

#if 0
	//for test , IR must be PowerUp
	{
		.seq_type = SENSOR_AVDD,
		.config_val = LDO_VOLTAGE_1P8V,  //1.8V
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
    },
#endif
	//VCM0[PMIC] 3V    //PMU_LDO25
    {
		.seq_type = SENSOR_VCM_AVDD,
		.config_val = 3000000,  //3V
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
    },

	//VCM1[PMIC] 3V     //PMU_LDO13
	{
		.seq_type = SENSOR_VCM_AVDD2,
		.config_val = 3000000,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},

	//MCAM0 AVDD0 2.85V[PMIC] //PMIC_LDO1
    {
		.seq_type = SENSOR_PMIC,
		.seq_val = VOUT_LDO_1,
		.config_val = LDO_VOLTAGE_V2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
    },

	//MCAM1 AVDD1 2.85V[PMIC] //PMIC_LDO2
	{
		.seq_type = SENSOR_PMIC,
		.seq_val = VOUT_LDO_2,
		.config_val = LDO_VOLTAGE_V2P8V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},

	//MCAM0 DVDD0 1.1V [LDO] //PMU_LDO20(VOUT20)
    {
        .seq_type = SENSOR_DVDD,
		.data = (void*)"main-sensor-dvdd0",
        .config_val = LDO_VOLTAGE_V1P1V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

	//MCAM1 DVDD1 1.1V [LDO] //PMU_LDO32
	{
		.seq_type = SENSOR_DVDD2,
		.data = (void*)"main-sensor-dvdd1",
		.config_val = LDO_VOLTAGE_V1P1V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},

	//MINIISP VDD Core 1.1V //PMIC_BUCK1
	{
		.seq_type = SENSOR_PMIC,
		.seq_val = VOUT_BUCK_1,
		.config_val = LDO_VOLTAGE_V1P1V,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},

	//MINIISP VDD PLL 1.1V   v1:HI6402_GPIO_23
	{
		.seq_type = SENSOR_MISPDCDC_EN,
		.data = (void*)"misp-pll-en",
		.config_val = SENSOR_GPIO_LOW,
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
	},

	//MINIISP & MCAM0 & MCAM1 IOVDD camera PMIC LDO4
	{
		.seq_type = SENSOR_PMIC,
		.seq_val = VOUT_LDO_4,
		.config_val = LDO_VOLTAGE_1P8V,  //1.8V
		.sensor_index = SENSOR_INDEX_INVALID,
		.delay = 1,
    },


	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = 0,
		.delay = 1,
	},

	{
		.seq_type = SENSOR_MCLK,
		.sensor_index = 1,
		.delay = 1,
	},
   //MCAM1 RESET0 //GPIO_018_MCAM0_RST
   {
       .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

	//MCAM2 RESET0 //GPIO_134_MCAM0_RST
	{
        .seq_type = SENSOR_RST2,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
};

static sensor_t s_imx214dual =
{
    .intf = { .vtbl = &s_imx214dual_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hw_imx214dual_power_setting),
            .power_setting = hw_imx214dual_power_setting,
     },
};

static const struct of_device_id
s_imx214dual_dt_match[] =
{
	{
        .compatible = "huawei,imx214d_knt",
        .data = &s_imx214dual.intf,
    },
	{
    },
};

MODULE_DEVICE_TABLE(of, s_imx214dual_dt_match);

static struct platform_driver
s_imx214dual_driver =
{
	.driver =
    {
		.name = "huawei,imx214d_knt",
		.owner = THIS_MODULE,
		.of_match_table = s_imx214dual_dt_match,
	},
};

static char const*
imx214dual_get_name(
        hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    return sensor->board_info->name;
}

static int
imx214dual_power_up(
        hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    sensor = I2S(si);
	cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
	if (hw_is_fpga_board()) {
		ret = do_sensor_power_on(sensor->board_info->sensor_index, sensor->board_info->name);
	} else {
		ret = hw_sensor_power_up(sensor);
	}
    if (0 == ret )
    {
        cam_info("%s. power up sensor success.", __func__);
    }
    else
    {
        cam_err("%s. power up sensor fail.", __func__);
    }
    return ret;
}

static int
imx214dual_power_down(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
	if (hw_is_fpga_board()) {
		ret = do_sensor_power_off(sensor->board_info->sensor_index, sensor->board_info->name);
	} else {
		ret = hw_sensor_power_down(sensor);
	}
    if (0 == ret )
    {
        cam_info("%s. power down sensor success.", __func__);
    }
    else
    {
        cam_err("%s. power down sensor fail.", __func__);
    }
	return ret;
}

static int imx214dual_csi_enable(hwsensor_intf_t* si)
{
    return 0;
}

static int imx214dual_csi_disable(hwsensor_intf_t* si)
{
    return 0;
}

static int
imx214dual_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = I2S(si);
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;

    cam_info("%s TODO.", __func__);

    strncpy(cdata->cfg.name, sensor->board_info->name, DEVICE_NAME_SIZE-1);
    cdata->data = sensor->board_info->sensor_index;

    return 0;
}

#if 0
static ssize_t imx214dual_powerctrl_show(struct device *dev,
	struct device_attribute *attr,char *buf)
{
    cam_info("enter %s", __func__);
    return 1;
}
static ssize_t imx214dual_powerctrl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int state = simple_strtol(buf, NULL, 10);
	cam_info("enter %s, state %d", __func__, state);

	if (state == POWER_ON)
		imx214dual_power_up(&s_imx214dual.intf);
	else
		imx214dual_power_down(&s_imx214dual.intf);

	return count;
}


static struct device_attribute imx214dual_powerctrl =
    __ATTR(power_ctrl, 0664, imx214dual_powerctrl_show, imx214dual_powerctrl_store);

static int imx214dual_register_attribute(hwsensor_intf_t* intf, struct device* dev)
{
	int ret = 0;
	cam_info("enter %s", __func__);

	ret = device_create_file(dev, &imx214dual_powerctrl);
	if (ret < 0) {
		cam_err("%s failed to creat power ctrl attribute.", __func__);
		goto err_create_power_ctrl;
	}
	return 0;
err_create_power_ctrl:
	device_remove_file(dev, &imx214dual_powerctrl);
	return ret;
}
#endif

static hwsensor_vtbl_t
s_imx214dual_vtbl =
{
	.get_name = imx214dual_get_name,
	.config = imx214dual_config,
	.power_up = imx214dual_power_up,
	.power_down = imx214dual_power_down,
	.match_id = imx214dual_match_id,
	.csi_enable = imx214dual_csi_enable,
	.csi_disable = imx214dual_csi_disable,
	/* .sensor_register_attribute = imx214dual_register_attribute, */
};

static int
imx214dual_config(
        hwsensor_intf_t* si,
        void  *argp)
{
	struct sensor_cfg_data *data;
	int ret =0;

	if (NULL == si || NULL == argp){
		cam_err("%s : si or argp is null", __func__);
		return -1;
	}

	data = (struct sensor_cfg_data *)argp;
	cam_debug("imx214dual cfgtype = %d",data->cfgtype);
	switch(data->cfgtype){
		case SEN_CONFIG_POWER_ON:
			ret = si->vtbl->power_up(si);
			break;
		case SEN_CONFIG_POWER_OFF:
			ret = si->vtbl->power_down(si);
			break;
		case SEN_CONFIG_WRITE_REG:
			break;
		case SEN_CONFIG_READ_REG:
			break;
		case SEN_CONFIG_WRITE_REG_SETTINGS:
			break;
		case SEN_CONFIG_READ_REG_SETTINGS:
			break;
		case SEN_CONFIG_ENABLE_CSI:
			break;
		case SEN_CONFIG_DISABLE_CSI:
			break;
		case SEN_CONFIG_MATCH_ID:
			ret = si->vtbl->match_id(si,argp);
			break;
		default:
			cam_err("%s cfgtype(%d) is error", __func__, data->cfgtype);
			break;
	}
	return ret;
}

static int32_t
imx214dual_platform_probe(
        struct platform_device* pdev)
{
	int rc = 0;
	cam_notice("enter %s",__func__);
	if (pdev->dev.of_node) {
		rc = hw_sensor_get_dt_data(pdev, &s_imx214dual);
		if (rc < 0) {
			cam_err("%s failed line %d\n", __func__, __LINE__);
			goto imx214dual_sensor_probe_fail;
		}
	} else {
		cam_err("%s imx214dual of_node is NULL.\n", __func__);
		goto imx214dual_sensor_probe_fail;
	}

    s_imx214dual.dev = &pdev->dev;

	rc = hwsensor_register(pdev, &s_imx214dual.intf);
	rc = rpmsg_sensor_register(pdev, (void*)&s_imx214dual);

imx214dual_sensor_probe_fail:
	return rc;
}

static int __init
imx214dual_init_module(void)
{
    cam_notice("enter %s",__func__);
    return platform_driver_probe(&s_imx214dual_driver,
            imx214dual_platform_probe);
}

static void __exit
imx214dual_exit_module(void)
{
    rpmsg_sensor_unregister((void*)&s_imx214dual);
    hwsensor_unregister(&s_imx214dual.intf);
    platform_driver_unregister(&s_imx214dual_driver);
}

module_init(imx214dual_init_module);
module_exit(imx214dual_exit_module);
MODULE_DESCRIPTION("imx214dual");
MODULE_LICENSE("GPL v2");
