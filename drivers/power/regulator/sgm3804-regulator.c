#include <common.h>
#include <dm.h>
#include <errno.h>
#include <i2c.h>
#include <asm/gpio.h>
#include <power/regulator.h>
#include <linux/delay.h>

struct sgm3804_priv {
	bool enabled;
	struct gpio_desc reset_gpio[2];
};

static int sgm3804_reg_write(struct udevice *dev, u8 reg, u8 val)
{
	int ret;

	ret = dm_i2c_write(dev, reg, &val, 1);
	if (ret) {
		dev_err(dev, "i2c write reg 0x%02x failed: %d\n", reg, ret);
		return ret;
	}
	return 0;
}

static int sgm3804_regulator_set_enable(struct udevice *dev, bool enable)
{
	struct sgm3804_priv *priv = dev_get_priv(dev);
	int ret;

	if (enable) {
		if (dm_gpio_is_valid(&priv->reset_gpio[0]))
			dm_gpio_set_value(&priv->reset_gpio[0], 1);
		if (dm_gpio_is_valid(&priv->reset_gpio[1]))
			dm_gpio_set_value(&priv->reset_gpio[1], 1);

		udelay(500);

		ret = sgm3804_reg_write(dev, 0x00, 0x0c);
		if (ret)
			return ret;

		ret = sgm3804_reg_write(dev, 0x01, 0x0c);
		if (ret)
			return ret;

		ret = sgm3804_reg_write(dev, 0x03, 0x03);
		if (ret)
			return ret;

		priv->enabled = true;
	} else {
		if (dm_gpio_is_valid(&priv->reset_gpio[0]))
			dm_gpio_set_value(&priv->reset_gpio[0], 0);
		if (dm_gpio_is_valid(&priv->reset_gpio[1]))
			dm_gpio_set_value(&priv->reset_gpio[1], 0);

		priv->enabled = false;
	}

	return 0;
}

static int sgm3804_regulator_get_enable(struct udevice *dev)
{
	struct sgm3804_priv *priv = dev_get_priv(dev);
	return priv->enabled ? 1 : 0;
}


static int sgm3804_regulator_get_value(struct udevice *dev)
{
	return 5000000;
}

static int sgm3804_regulator_ofdata_to_platdata(struct udevice *dev)
{
	struct sgm3804_priv *priv = dev_get_priv(dev);
	int ret;

	ret = gpio_request_by_name(dev, "reset-gpios", 0, &priv->reset_gpio[0],
				   GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "failed to get reset-gpios[0]: %d\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "reset-gpios", 1, &priv->reset_gpio[1],
				   GPIOD_IS_OUT);
	if (ret)
		debug("sgm3804: optional reset-gpios[1] not found: %d\n", ret);

	if (dm_gpio_is_valid(&priv->reset_gpio[0]))
		dm_gpio_set_value(&priv->reset_gpio[0], 0);
	if (dm_gpio_is_valid(&priv->reset_gpio[1]))
		dm_gpio_set_value(&priv->reset_gpio[1], 0);

	priv->enabled = false;

	return 0;
}

static int sgm3804_regulator_probe(struct udevice *dev)
{
	return 0;
}

static const struct dm_regulator_ops sgm3804_regulator_ops = {
	.get_value  = sgm3804_regulator_get_value,
	.set_enable = sgm3804_regulator_set_enable,
	.get_enable = sgm3804_regulator_get_enable,
};

static const struct udevice_id sgm3804_ids[] = {
	{ .compatible = "sgmicro,sgm3804" },
	{ }
};

U_BOOT_DRIVER(sgm3804_regulator) = {
	.name = "sgm3804_regulator",
	.id = UCLASS_REGULATOR,
	.of_match = sgm3804_ids,
	.ops = &sgm3804_regulator_ops,
	.probe = sgm3804_regulator_probe,
	.ofdata_to_platdata = sgm3804_regulator_ofdata_to_platdata,
	.priv_auto_alloc_size = sizeof(struct sgm3804_priv),
};
