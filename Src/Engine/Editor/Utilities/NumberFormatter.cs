using System;
using System.Globalization;

namespace SE.Editor
{
    public static class NumberFormatter
    {
        public static string FormatFloat(float value, Utils.ValueCategory category)
        {
            if (float.IsPositiveInfinity(value) || value == float.MaxValue)
            {
                return "Infinity";
            }
            if (float.IsNegativeInfinity(value) || value == float.MinValue)
            {
                return "-Infinity";
            }
            return Format(value, "G7", category);
        }

        public static string FormatFloat(double value, Utils.ValueCategory category)
        {
            return Format(value, "G15", category);
        }

        private static string Format(double value, string format, Utils.ValueCategory category)
        {
            if (double.IsPositiveInfinity(value) || value == double.MaxValue)
            {
                return "Infinity";
            }
            if (double.IsNegativeInfinity(value) || value == double.MinValue)
            {
                return "-Infinity";
            }
            if (!Units.UseUnitsFormatting || category == Utils.ValueCategory.None)
            {
                return value.ToString(format, CultureInfo.InvariantCulture);
            }
            double units = Units.Meters2Units;
            string Number(double v) => v.ToString(format, CultureInfo.InvariantCulture);
            return category switch
            {
                Utils.ValueCategory.Distance when !Units.AutomaticUnitsFormatting => Number(value / units) + Units
                    .Unit("m"),
                Utils.ValueCategory.Distance when Math.Abs(value) < units => Number(value) + Units.Unit("cm"),
                Utils.ValueCategory.Distance when Math.Abs(value) < units * 1000 => Number(value / units) + Units
                    .Unit("m"),
                Utils.ValueCategory.Distance => Number(value / units / 1000) + Units.Unit("km"),
                Utils.ValueCategory.Angle => Number(value) + "°",
                Utils.ValueCategory.Time => Number(value) + Units.Unit("s"),
                Utils.ValueCategory.Speed => Number(value / units) + Units.Unit("m/s"),
                Utils.ValueCategory.Acceleration => Number(value / units) + Units.Unit("m/s²"),
                Utils.ValueCategory.Area => Number(value / units / units) + Units.Unit("m²"),
                Utils.ValueCategory.Volume => Number(value / units / units / units) + Units.Unit("m³"),
                Utils.ValueCategory.Mass => Number(value) + Units.Unit("kg"),
                Utils.ValueCategory.Force => Number(value / units) + Units.Unit("N"),
                Utils.ValueCategory.Torque => Number(value / units / units) + Units.Unit("Nm"),
                _ => Number(value),
            };
        }
    }
}
