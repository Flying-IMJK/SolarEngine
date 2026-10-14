namespace SE.Editor.Utilities
{
    public static class Units
    {
        public static readonly float Meters2Units = 100f;
        public static bool UseUnitsFormatting = true;
        public static bool SeparateValueAndUnit = true;
        public static bool AutomaticUnitsFormatting = true;
        public static string Unit(string unit) => SeparateValueAndUnit ? $" {unit}" : unit;
    }
}
