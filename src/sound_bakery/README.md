# Sound Bakery

Sound Bakery is the core audio library for creating interactive audio experiences. Its design is very similar to Wwise. Almost everything extends the sbk::core::object class. Lots of objects extend the sbk::core::database_object class, allowing the object to be searchable by ID and name.

Sound Bakery can easily be thought of as a database with runtime components that read from that database. Anything that extends sbk::core::database_object is part of the database.

The library uses concurrencpp to easily run code on different threads.

The library also comes with reflection, thanks to rttr.